#include "network/Server.h"
#include "components/EventSystem.h"
#include "components/MatchSettings.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ServerConsoleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/WorldSnapshot.h"
#include "network/DatagramLink.h"
#include "network/ReplicationBindings.h"
#include "network/Serializer.h"
#include "network/WireFrame.h"
#include "network/commands/CommandBatch.h"
#include "network/commands/Disconnect.h"
#include "enums/DisconnectReason.h"
#include "enums/MatchRules.h"
#include "enums/PlayerSlot.h"
#include "enums/GameState.h"
#include "utils/Log.h"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <boost/asio/strand.hpp>
#include <boost/asio/post.hpp>
#include <memory>
#include <mutex>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <variant>
#include <vector>

namespace
{
//NOTE: one to four seats, whatever the launch asked for, and a bot short of filling them all
MatchSettings Seated(MatchSettings match)
{
	match.seats = std::clamp(match.seats, std::uint8_t{1}, static_cast<std::uint8_t>(kSeatCount));
	match.bots = std::min(match.bots, MaxBots(match.seats));

	return match;
}
}//namespace

namespace network::commands
{
Server::Server(boost::asio::io_context& ioContext, const ServerAddress& address,
			   const std::shared_ptr<EventSystem>& events, const MatchSettings& match)
	: _socket{ioContext, udp::endpoint{boost::asio::ip::make_address(address.host), address.port}}
	, _boundPort{_socket.local_endpoint().port()}
	, _match{Seated(match)}
	, _beacon{ioContext, boost::asio::ip::make_address(address.host), _boundPort,
			  _match.seats, _match.rules, [this] { return CountFreeSlots(); }}
	, _tickTimer{ioContext}
	, _events{events}
	, _replicationOut{events}
{
	BindHostReplication(_replicationOut);

	boost::system::error_code ec;
	std::ignore = _socket.set_option(udp::socket::receive_buffer_size{DatagramLink::kSocketBufferSize}, ec);

	Receive();
	ScheduleTick();

	_subs.push_back(_events->AddListener(this, &Server::OnNetworkEndFrame));
	_subs.push_back(_events->AddListener(this, &Server::OnMapLoaded));
	_subs.push_back(_events->AddListener(this, &Server::OnSeatHolderChanged));
	_subs.push_back(_events->AddListener(this, &Server::OnStatusRequested));
	_subs.push_back(_events->AddListener(this, &Server::OnPlayersRequested));
	_subs.push_back(_events->AddListener(this, &Server::OnKickRequested));
	_subs.push_back(_events->AddListener(this, &Server::OnAcceptingChanged));
}

Server::~Server()// NOLINT(bugprone-exception-escape) - cancel() throws only on an error the timer service never sets
{
	Shutdown();
}

void Server::Shutdown() { CloseSocket(); }

void Server::Shutdown(const DisconnectReason reason, const std::function<void()>& onClosed)
{
	//NOTE: a finished session has nobody left to hear the goodbye
	auto sessions{CopySessions()};
	std::erase_if(sessions, [](const std::shared_ptr<Session>& session) { return session->IsFinished(); });
	if (sessions.empty() || !_socket.is_open())
	{
		CloseSocket();
		if (onClosed)
		{
			onClosed();
		}

		return;
	}

	const auto now{Clock::now()};
	for (const auto& session: sessions)
	{
		session->Shutdown(reason);
		Transmit(*session, now);
	}

	_onClosed = onClosed;
	_closeDeadline = now + DatagramLink::kFarewellLinger;
}

std::uint8_t Server::CountFreeSlots() const
{
	const std::lock_guard lock{_sessionsMutex};

	std::uint8_t free{};
	for (const PlayerSlot slot: kSlots | std::views::take(_match.seats))
	{
		const auto holdsSlot = [slot](const std::shared_ptr<Session>& session)
		{
			return !session->IsFinished() && session->GetSlot() == slot;
		};

		free += static_cast<std::uint8_t>(!std::ranges::any_of(_sessions, holdsSlot));
	}

	return free;
}

void Server::CloseSocket()
{
	_beacon.Shutdown();

	std::ignore = _tickTimer.cancel();

	if (!_socket.is_open())
	{
		return;
	}

	boost::system::error_code ec;
	std::ignore = _socket.cancel(ec);
	std::ignore = _socket.close(ec);
}

void Server::Receive()
{
	_socket.async_receive_from(boost::asio::buffer(_receiveBuffer), _sender,
							   [this](const boost::system::error_code& ec, const std::size_t size)
							   {
								   if (ec == boost::asio::error::operation_aborted || !_socket.is_open())
								   {
									   return;
								   }

								   //NOTE: one datagram's error - Windows reports a peer that went away on
								   //the next receive, and the socket itself is fine
								   if (!ec)
								   {
									   OnDatagram(std::string_view{_receiveBuffer.data(), size}, Clock::now());
								   }

								   Receive();
							   });
}

void Server::OnDatagram(const std::string_view datagram, const Clock::time_point now)
{
	const auto header{DatagramLink::PeekHeader(datagram)};
	if (!header)
	{
		return;
	}

	const auto sessions{CopySessions()};
	const auto isSameAddress = [this](const std::shared_ptr<Session>& session)
	{
		return session->Endpoint() == _sender;
	};

	const auto link{std::ranges::find_if(sessions, [&isSameAddress, &header](const std::shared_ptr<Session>& session)
	{
		return isSameAddress(session) && session->ConnectionId() == header->connectionId;
	})};

	if (link != sessions.end())
	{
		(*link)->Receive(datagram, now);
		Transmit(**link, now);
		return;
	}

	//NOTE: anything but a hello from an address without a link is left over from a connection that is gone
	if (!header->isHello || _onClosed)
	{
		return;
	}

	std::ranges::for_each(sessions | std::views::filter(isSameAddress),
						  [](const std::shared_ptr<Session>& session) { session->Supersede(); });

	Seat(_sender, header->connectionId, datagram, now);
}

void Server::RefuseSeat(const udp::endpoint& endpoint, const std::uint32_t connectionId, const Clock::time_point now)
{
	Log::Info("Server: every seat is taken, the client is told to wait for a free match");

	CommandBatch farewell;
	farewell.commands.emplace_back(Disconnect{.reason = DisconnectReason::ServerFull});

	//NOTE: no session is kept for it - the client asks again later, and every ask gets the same answer
	DatagramLink refusal{connectionId, now};
	std::ignore = refusal.SendReliable(network::Serialize(farewell));
	SendTo(endpoint, refusal.TakeDatagrams(now));
}

void Server::Seat(const udp::endpoint& endpoint, const std::uint32_t connectionId, const std::string_view hello,
				  const Clock::time_point now)
{
	//NOTE: a console that stopped taking clients answers like a full server - the dialler waits instead of
	//burning its retries on silence
	if (!_isAccepting.load(std::memory_order_acquire))
	{
		RefuseSeat(endpoint, connectionId, now);
		return;
	}

	std::unique_lock lock{_sessionsMutex};
	const auto slot{FindFreeSlot(endpoint)};
	if (!slot)
	{
		lock.unlock();
		RefuseSeat(endpoint, connectionId, now);
		return;
	}

	const auto session{std::make_shared<Session>(endpoint, connectionId, _events, *slot, _match, now)};
	_sessions.emplace_back(session);
	_seatHistory.Seat(*slot, endpoint);
	lock.unlock();

	session->Start();
	session->Receive(hello, now);
	Transmit(*session, now);
}

void Server::ScheduleTick()
{
	_tickTimer.expires_after(DatagramLink::kPollInterval);
	_tickTimer.async_wait([this](const boost::system::error_code& ec)
	{
		if (!ec)
		{
			Tick();
		}
	});
}

void Server::Tick()
{
	if (!_socket.is_open())
	{
		return;
	}

	const auto now{Clock::now()};
	const auto sessions{CopySessions()};
	for (const auto& session: sessions)
	{
		session->LoseIfSilent(now);
		Transmit(*session, now);
	}

	const auto isDrained = [](const std::shared_ptr<Session>& session) { return session->IsDrained(); };
	if (_onClosed && (now >= _closeDeadline || std::ranges::all_of(sessions, isDrained)))
	{
		const auto onClosed{std::move(_onClosed)};
		_onClosed = nullptr;
		CloseSocket();
		onClosed();

		return;
	}

	ScheduleTick();
}

void Server::Transmit(Session& session, const Clock::time_point now)
{
	SendTo(session.Endpoint(), session.TakeDatagrams(now));
}

void Server::SendTo(const udp::endpoint& endpoint, const std::vector<std::string>& datagrams)
{
	for (const std::string& datagram: datagrams)
	{
		boost::system::error_code ec;
		std::ignore = _socket.send_to(boost::asio::buffer(datagram), endpoint, 0, ec);
		if (ec)
		{
			Log::Error("Server send: " + ec.message());
		}
	}
}

void Server::OnStatusRequested(const ServerStatusRequestedEvent&) const
{
	const auto sessions{CopySessions()};
	const auto seated{std::ranges::count_if(sessions, [](const std::shared_ptr<Session>& session)
	{
		return !session->IsFinished();
	})};

	Log::Info("port " + std::to_string(_boundPort)
			  + (_isAccepting.load(std::memory_order_acquire) ? " open" : " closed") + ", seats taken "
			  + std::to_string(seated) + '/' + std::to_string(_match.seats));
}

void Server::OnPlayersRequested(const ServerPlayersRequestedEvent&) const
{
	const auto sessions{CopySessions()};
	if (sessions.empty())
	{
		Log::Info("no players");

		return;
	}

	const auto now{std::chrono::steady_clock::now()};
	std::ranges::for_each(sessions, [now](const std::shared_ptr<Session>& session)
	{
		const auto connected{std::chrono::duration_cast<std::chrono::seconds>(now - session->ConnectedAt())};
		Log::Info(std::string{ToString(session->GetSlot())} + " " + session->Address() + ", connected "
				  + std::to_string(connected.count()) + "s" + (session->IsFinished() ? ", leaving" : ""));
	});
}

void Server::OnKickRequested(const ServerKickRequestedEvent& event) const
{
	const auto sessions{CopySessions()};
	const auto kicked{std::ranges::find_if(sessions, [slot = event.slot](const std::shared_ptr<Session>& session)
	{
		return !session->IsFinished() && session->GetSlot() == slot;
	})};

	if (kicked == sessions.end())
	{
		Log::Info("nobody sits in " + std::string{ToString(event.slot)});

		return;
	}

	Log::Info("kicking " + std::string{ToString(event.slot)} + " " + (*kicked)->Address());
	(*kicked)->Kick();
}

void Server::OnAcceptingChanged(const ServerAcceptingChangedEvent& event)
{
	_isAccepting.store(event.isAccepting, std::memory_order_release);

	Log::Info(event.isAccepting ? "taking new clients" : "not taking new clients");
}

//NOTE: a snapshot goes out in place of the frame, never beside it - it already holds everything the frame says
void Server::OnNetworkEndFrame(const NetworkEndFrameEvent&)
{
	CleanupDeadSessions();

	//NOTE: archived here, on the game thread - only the datagrams leave on the io thread
	const auto frame{_replicationOut.TakeFrame()};
	std::optional<std::shared_ptr<const WireFrame>> snapshot{};

	for (const auto& session: CopySessions())
	{
		std::shared_ptr<const WireFrame> outgoing{frame};
		if (session->IsSnapshotOwed())
		{
			if (!snapshot)
			{
				snapshot = TakeWorldSnapshot();
			}

			//NOTE: a lobby has no field, and the debt stands until there is one - cleared here it would
			//leave whoever readied first playing on a field nobody ever sent him
			if (*snapshot)
			{
				session->ClearSnapshotDebt();
				outgoing = *snapshot;
			}
		}

		if (outgoing)
		{
			boost::asio::post(_socket.get_executor(), [this, session, outgoing]
			{
				session->Send(*outgoing);
				Transmit(*session, Clock::now());
			});
		}
	}
}

//NOTE: the next level is played on a map the launch never named
void Server::OnMapLoaded(const MapLoadedEvent& event)
{
	const std::scoped_lock lock{_sessionsMutex};
	_match.map = event.name;
}

//NOTE: everyone, not only the one who sat down - the seat's lives may have been given back, and only the
//field says so. The match is held while it goes out, so nobody sees it jump
void Server::OnSeatHolderChanged(const SeatHolderChangedEvent&) const
{
	std::ranges::for_each(CopySessions(), [](const std::shared_ptr<Session>& session) { session->OweSnapshot(); });
}

std::shared_ptr<const WireFrame> Server::TakeWorldSnapshot() const
{
	CommandBatch batch;
	auto& snapshot{std::get<WorldSnapshot>(batch.commands.emplace_back(WorldSnapshot{}))};
	_events->EmitEvent(WorldSnapshotRequestedEvent{.snapshot = snapshot});

	if (!IsInMatch(snapshot.phase))
	{
		return nullptr;
	}

	return std::make_shared<const WireFrame>(
			WireFrame{.reliable = network::Serialize(batch), .latest = {}, .isSnapshot = true});
}

std::vector<std::shared_ptr<Session>> Server::CopySessions() const
{
	const std::scoped_lock lock{_sessionsMutex};
	return _sessions;
}

std::optional<PlayerSlot> Server::FindFreeSlot(const udp::endpoint& endpoint) const
{
	//NOTE: a finished session gives its seat up at once - a client dialling back takes it before the sweep
	const auto isFree = [this](const PlayerSlot slot)
	{
		return std::ranges::none_of(_sessions, [slot](const std::shared_ptr<Session>& session)
		{
			return !session->IsFinished() && session->GetSlot() == slot;
		});
	};

	const auto freeSeats{kSlots | std::views::take(_match.seats) | std::views::filter(isFree)
						 | std::ranges::to<std::vector>()};

	return _seatHistory.Choose(freeSeats, endpoint);
}

void Server::CleanupDeadSessions()
{
	//NOTE: a finished session is not dead while its goodbye is unread, or while its own is still in flight -
	//that datagram leaves on the tick. Silence is the backstop for a peer that stopped acking
	const auto now{Clock::now()};
	const auto isDead = [now](const std::shared_ptr<Session>& session)
	{
		return session->IsFinished() && !session->HasPendingCommands()
			   && (session->IsDrained() || session->IsSilent(now));
	};

	const std::scoped_lock lock{_sessionsMutex};
	std::erase_if(_sessions, isDead);
}

void Server::ProcessNetworkCommands() const
{
	//NOTE: drained even when finished - commands already read stay valid, and the last is the goodbye
	for (const auto& session: CopySessions())
	{
		session->ProcessCommandQueue();
	}
}
}//namespace network::commands
