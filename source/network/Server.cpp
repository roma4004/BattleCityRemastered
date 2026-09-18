#include "network/Server.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ServerConsoleEvents.h"
#include "network/FrameChannel.h"
#include "network/ReplicationBindings.h"
#include "network/Serializer.h"
#include "network/commands/CommandBatch.h"
#include "network/commands/Disconnect.h"
#include "enums/DisconnectReason.h"
#include "enums/PlayerSlot.h"
#include <memory>
#include "utils/Log.h"
#include <algorithm>
#include <chrono>
#include <boost/asio/strand.hpp>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace network::commands
{
Server::Server(boost::asio::io_context& ioContext, const ServerAddress& address,
			   const std::shared_ptr<EventSystem>& events)
	: _acceptor{tcp::acceptor(ioContext,
							  tcp::endpoint(boost::asio::ip::make_address(address.host), address.port))}
	, _endpoint{_acceptor.local_endpoint()}
	, _events{events}
	, _replicationOut{events}
{
	BindHostReplication(_replicationOut);
	DoAccept();

	_subs.push_back(_events->AddListener(this, &Server::OnNetworkEndFrame));
	_subs.push_back(_events->AddListener(this, &Server::OnStatusRequested));
	_subs.push_back(_events->AddListener(this, &Server::OnPlayersRequested));
	_subs.push_back(_events->AddListener(this, &Server::OnKickRequested));
	_subs.push_back(_events->AddListener(this, &Server::OnAcceptingChanged));
}

Server::~Server()
{
	Shutdown();
}

void Server::Shutdown()
{
	CloseAcceptor();

	for (const auto& session: SnapshotSessions())
	{
		if (session)
		{
			session->Shutdown();
		}
	}
}

void Server::Shutdown(const DisconnectReason reason, const std::function<void()>& onClosed)
{
	CloseAcceptor();

	const auto sessions{SnapshotSessions()};
	//NOTE: counted, not per-session - the caller is told once, after the last goodbye is out
	const auto pending{std::make_shared<std::size_t>(sessions.size())};

	if (sessions.empty())
	{
		if (onClosed)
		{
			onClosed();
		}
		return;
	}

	for (const auto& session: sessions)
	{
		if (!session)
		{
			--*pending;
			continue;
		}

		session->Shutdown(reason, [pending, onClosed]
		{
			if (--*pending == 0u && onClosed)
			{
				onClosed();
			}
		});
	}
}

void Server::CloseAcceptor()
{
	if (!_acceptor.is_open())
	{
		return;
	}

	boost::system::error_code ec;
	std::ignore = _acceptor.cancel(ec);
	std::ignore = _acceptor.close(ec);
}

//NOTE: the port it had - a client dialling the same address finds it again
void Server::OpenAcceptor()
{
	if (_acceptor.is_open())
	{
		return;
	}

	boost::system::error_code ec;
	if (_acceptor.open(_endpoint.protocol(), ec)
		|| _acceptor.set_option(tcp::acceptor::reuse_address(true), ec)
		|| _acceptor.bind(_endpoint, ec)
		|| _acceptor.listen(boost::asio::socket_base::max_listen_connections, ec))
	{
		Log::Error("Server reopening port " + std::to_string(_endpoint.port()) + ": " + ec.message());
		CloseAcceptor();

		return;
	}

	DoAccept();
}

void Server::OnStatusRequested(const ServerStatusRequestedEvent&) const
{
	const auto sessions{SnapshotSessions()};
	const auto seated{std::ranges::count_if(sessions, [](const std::shared_ptr<Session>& session)
	{
		return session && !session->IsFinished();
	})};

	Log::Info("port " + std::to_string(_endpoint.port()) + (_acceptor.is_open() ? " open" : " closed")
			  + ", seats taken " + std::to_string(seated) + "/2");
}

void Server::OnPlayersRequested(const ServerPlayersRequestedEvent&) const
{
	const auto sessions{SnapshotSessions()};
	if (sessions.empty())
	{
		Log::Info("no players");

		return;
	}

	const auto now{std::chrono::steady_clock::now()};
	std::ranges::for_each(sessions, [now](const std::shared_ptr<Session>& session)
	{
		if (!session)
		{
			return;
		}

		const auto connected{std::chrono::duration_cast<std::chrono::seconds>(now - session->ConnectedAt())};
		Log::Info(std::string{ToString(session->GetSlot())} + " " + session->Address() + ", connected "
				  + std::to_string(connected.count()) + "s" + (session->IsFinished() ? ", leaving" : ""));
	});
}

void Server::OnKickRequested(const ServerKickRequestedEvent& event) const
{
	const auto sessions{SnapshotSessions()};
	const auto kicked{std::ranges::find_if(sessions, [slot = event.slot](const std::shared_ptr<Session>& session)
	{
		return session && !session->IsFinished() && session->GetSlot() == slot;
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
	if (event.isAccepting)
	{
		OpenAcceptor();
	}
	else
	{
		CloseAcceptor();
	}

	Log::Info(_acceptor.is_open() ? "taking new clients" : "not taking new clients");
}

void Server::OnNetworkEndFrame(const NetworkEndFrameEvent&)
{
	CleanupDeadSessions();

	//NOTE: serialised here, on the game thread, exactly as Client does it - FrameChannel::Send only
	//posts onto the session's strand, so nothing here waits on the socket
	if (const auto frame{_replicationOut.TakeFrame()})
	{
		SendToAll(frame);
	}
}

//NOTE: a bare EOF reads as a dropped link - told why, the client waits instead of burning its retries
void Server::RefuseSeat(tcp::socket socket) const
{
	Log::Info("Server: both seats are taken, the client is told to wait for a free match");

	CommandBatch farewell;
	farewell.commands.emplace_back(Disconnect{.reason = DisconnectReason::ServerFull});

	//NOTE: the channel keeps itself alive through the write it posted, so this handle may go
	const auto channel{std::make_shared<network::FrameChannel>(std::move(socket), "Server")};
	channel->Send(std::make_shared<const std::string>(network::SerializeFrame(farewell)));
	channel->CloseAfterFlush({});
}

void Server::Seat(tcp::socket socket)
{
	try
	{
		boost::system::error_code ec;
		const auto remote{socket.remote_endpoint(ec)};
		std::string address{ec ? "unknown" : remote.address().to_string() + ':' + std::to_string(remote.port())};

		std::unique_lock lock(_sessionsMutex);
		const auto slot{FindFreeSlot()};
		if (!slot)
		{
			lock.unlock();
			RefuseSeat(std::move(socket));
			return;
		}

		const auto session{std::make_shared<Session>(std::move(socket), _events, *slot, std::move(address))};
		_sessions.emplace_back(session);
		lock.unlock();

		session->Start();//NOTE: outside the lock - it posts reads and can reach the event bus
	}
	catch (const std::exception& e)
	{
		Log::Error(std::string("Server new session start: ") + e.what());
	}
	catch (...)
	{
		Log::Error("Server new session start: unknown exception");
	}
}

void Server::DoAccept()
{
	const auto executor{boost::asio::make_strand(_acceptor.get_executor())};
	//NOTE: own strand per socket - serializes that session's handlers against each other
	_acceptor.async_accept(executor, [this](const boost::system::error_code& ec, tcp::socket socket)
	{
		if (ec)
		{
			if (ec != boost::asio::error::operation_aborted)
			{
				Log::Error("Server accept: " + ec.message());
			}

			return;
		}

		Seat(std::move(socket));
		DoAccept();
	});
}

std::vector<std::shared_ptr<Session>> Server::SnapshotSessions() const
{
	const std::scoped_lock lock{_sessionsMutex};
	return _sessions;
}

std::optional<PlayerSlot> Server::FindFreeSlot() const
{
	for (const PlayerSlot slot: {PlayerSlot::P1, PlayerSlot::P2})
	{
		const auto holdsSlot = [slot](const std::shared_ptr<Session>& session)
		{
			return session && session->GetSlot() == slot;
		};

		if (!std::ranges::any_of(_sessions, holdsSlot))
		{
			return slot;
		}
	}

	return std::nullopt;
}

void Server::CleanupDeadSessions()
{
	//NOTE: a finished session with commands still queued is not dead yet - its goodbye is unread
	const auto isDead = [](const std::shared_ptr<Session>& session)
	{
		return !session || (session->IsFinished() && !session->HasPendingCommands());
	};

	//NOTE: ~Session only posts the close onto its strand, so it costs nothing to let it happen here
	const std::scoped_lock lock{_sessionsMutex};
	std::erase_if(_sessions, isDead);
}

void Server::SendToAll(const std::shared_ptr<const std::string>& message)
{
	//NOTE: nothing asks whether the link is up - DoWrite posts onto the session strand, and
	//TryStartWrite decides there, on the thread that owns the socket
	for (const auto& session: SnapshotSessions())
	{
		if (session)
		{
			session->DoWrite(message);
		}
	}
}

void Server::ProcessNetworkCommands() const
{
	for (const auto& session: SnapshotSessions())
	{
		//NOTE: drained even when finished - frames already read stay valid, and the last is the goodbye
		if (session)
		{
			session->ProcessCommandQueue();
		}
	}
}
}//namespace network::commands
