#include "network/Client.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "enums/ClientSignal.h"
#include "network/DatagramLink.h"
#include "network/ReplicationBindings.h"
#include "network/Serializer.h"
#include "network/commands/CommandBatch.h"
#include "utils/Log.h"
#include "utils/RandUtils.h"
#include <boost/asio/post.hpp>
#include <cstdint>
#include <random>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>

namespace
{
//NOTE: a server told to listen everywhere is dialled on this machine - 0.0.0.0 is not a place to connect to
boost::asio::ip::address DialAddress(const std::string& host)
{
	auto address{boost::asio::ip::make_address(host)};
	if (address.is_unspecified())
	{
		address = address.is_v6() ? boost::asio::ip::address{boost::asio::ip::address_v6::loopback()}
								  : boost::asio::ip::address{boost::asio::ip::address_v4::loopback()};
	}

	return address;
}
}//namespace

namespace network::commands
{
Client::Client(boost::asio::io_context& ioContext, const ServerAddress& address,
			   const std::shared_ptr<EventSystem>& events)
	: PeerLink("Client", events)
	, _endpoint{DialAddress(address.host), address.port}
	, _socket{ioContext, _endpoint.protocol()}
	, _tickTimer{ioContext}
	, _reconnectTimer{ioContext}
	, _replicationIn{events, _commandQueue}
	, _replicationOut{events}
{
	boost::system::error_code ec;
	std::ignore = _socket.set_option(udp::socket::receive_buffer_size{DatagramLink::kSocketBufferSize}, ec);
	//NOTE: connected, so the socket hands over datagrams from the host alone
	std::ignore = _socket.connect(_endpoint, ec);
	if (ec)
	{
		Log::Error("Client connect: " + ec.message());
	}

	BindClientReplication(_replicationOut);
	Subscribe();

	Receive();
	ScheduleTick();
	TryConnect();
}

void Client::OnCommand(const AnyCommand& command)
{
	//NOTE: peeled off rather than left to the applier - the goodbye has to be read on the network thread,
	//while everything else is a game fact and belongs on the game one
	if (const auto* goodbye{std::get_if<Disconnect>(&command)})
	{
		OnDisconnect(*goodbye);
		return;
	}

	_replicationIn.Apply(command);
}

void Client::TryConnect()
{
	const auto now{Clock::now()};
	_link.emplace(RandUtils::GetRandNumber(std::uniform_int_distribution<std::uint32_t>{}), now);
	_attemptStartedAt = now;
	Transmit(now);
}

void Client::Receive()
{
	_socket.async_receive(boost::asio::buffer(_receiveBuffer),
						  [this](const boost::system::error_code& ec, const std::size_t size)
						  {
							  if (ec == boost::asio::error::operation_aborted || !_socket.is_open())
							  {
								  return;
							  }

							  //NOTE: one datagram's error - Windows refuses on the next receive when
							  //nobody listens at the far end
							  if (!ec)
							  {
								  OnDatagram(std::string_view{_receiveBuffer.data(), size}, Clock::now());
							  }

							  Receive();
						  });
}

void Client::OnDatagram(const std::string_view datagram, const Clock::time_point now)
{
	if (!_link)
	{
		return;
	}

	const auto arrivals{_link->Receive(datagram, now)};
	if (!arrivals)
	{
		return;
	}

	const bool isFirstAnswer{!_isConnected && !_isShuttingDown};

	Transmit(now);

	if (!Dispatch(*arrivals))
	{
		HandleProtocolError();
		return;
	}

	//NOTE: announced after the batch, not before - a refusal rides in the very datagram that answers the
	//hello, and the link is gone by now, so a turned-away client never reports a seat it did not get
	if (!isFirstAnswer || !_link || _isShuttingDown)
	{
		return;
	}

	Log::Info("client connected");
	_reconnectAttempts = 0;
	_reconnectAbandoned = false;
	_isConnected = true;
	_commandQueue.Enqueue([this] { _events->EmitEvent(ClientConnectedToHostEvent{}); });
}

void Client::ScheduleTick()
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

void Client::Tick()
{
	if (!_socket.is_open())
	{
		return;
	}

	const auto now{Clock::now()};

	if (_onClosed)
	{
		if (now < _closeDeadline && _link && !_link->IsDrained())
		{
			Transmit(now);
			ScheduleTick();
			return;
		}

		const auto onClosed{std::move(_onClosed)};
		_onClosed = nullptr;
		CloseSocket();
		onClosed();

		return;
	}

	if (_link && !_isConnected && now - _attemptStartedAt > kConnectTimeout)
	{
		++_reconnectAttempts;
		Log::Error("Client connect failed (attempt " + std::to_string(_reconnectAttempts) + "/"
				   + std::to_string(kMaxReconnectAttempts) + "): the host did not answer");
		_link.reset();
		ScheduleReconnect();
	}
	else if (_link && _isConnected && _link->IsSilent(now))
	{
		Log::Error("Client: the host went silent");
		HandleDisconnect();
	}

	Transmit(now);
	ScheduleTick();
}

void Client::Transmit(const Clock::time_point now)
{
	if (!_link)
	{
		return;
	}

	for (const std::string& datagram: _link->TakeDatagrams(now))
	{
		boost::system::error_code ec;
		std::ignore = _socket.send(boost::asio::buffer(datagram), 0, ec);
		//NOTE: a refusal is nobody listening yet - the connect attempt runs out on its own
		if (ec && ec != boost::asio::error::connection_refused && ec != boost::asio::error::connection_reset)
		{
			Log::Error("Client send: " + ec.message());
		}
	}
}

void Client::CloseSocket()
{
	std::ignore = _tickTimer.cancel();

	if (!_socket.is_open())
	{
		return;
	}

	boost::system::error_code ec;
	std::ignore = _socket.cancel(ec);
	std::ignore = _socket.close(ec);
}

void Client::ScheduleReconnect()
{
	if (_reconnectPending || _isShuttingDown)
	{
		return;
	}

	if (_isLinkUnrecoverable || (!_isWaitingForSeat && _reconnectAttempts >= kMaxReconnectAttempts))
	{
		if (!_reconnectAbandoned)
		{
			_reconnectAbandoned = true;
			Log::Error("Client gave up on the host");
			_commandQueue.Enqueue([this] { _events->EmitEvent(ClientReconnectAbandonedEvent{}); });
		}
		return;
	}

	_reconnectPending = true;

	//NOTE: weak - the timer is our own member, so a shared capture would keep this Client alive
	//through its own pending handler
	const std::weak_ptr<Client> weakSelf{weak_from_this()};
	_reconnectTimer.expires_after(_isWaitingForSeat ? kFullServerRetry : kReconnectDelay);
	_reconnectTimer.async_wait([weakSelf](const boost::system::error_code& timerEc)
	{
		const auto self{weakSelf.lock()};
		if (!self)
		{
			return;
		}

		self->_reconnectPending = false;
		if (!timerEc && !self->_isShuttingDown)
		{
			self->TryConnect();
		}
	});
}

void Client::HandleDisconnect()
{
	if (_isShuttingDown || _reconnectPending)
	{
		return;
	}

	_isConnected = false;
	_link.reset();
	_commandQueue.Enqueue([this] { _events->EmitEvent(ClientHostLostEvent{}); });

	if (_isLinkUnrecoverable)
	{
		CloseSocket();
	}
	else
	{
		_reconnectAttempts = 0;//NOTE: a drop starts a fresh budget, it is not a failed connect attempt
	}

	ScheduleReconnect();
}

//NOTE: an unreadable message arrived whole and in order, so it is a protocol disagreement - the next one
//fails the same way, and the link ends here
void Client::HandleProtocolError()
{
	if (_isShuttingDown)
	{
		return;
	}

	_isConnected = false;
	_commandQueue.Enqueue([this]
	{
		_events->EmitEvent(ClientInDisconnectEvent{.reason = DisconnectReason::ProtocolError});
	});

	Shutdown();
}

Client::~Client()// NOLINT(bugprone-exception-escape) - cancel() throws only on an error the timer service never sets
{
	Shutdown();
}

void Client::Shutdown()
{
	_isShuttingDown = true;//NOTE: before cancelling - handlers must not read the cancel as a drop

	std::ignore = _reconnectTimer.cancel();//NOTE: no-throw, so ~Client is safe without a catch-all
	CloseSocket();
}

void Client::Shutdown(const DisconnectReason reason, std::function<void()> onClosed)
{
	_isShuttingDown = true;
	std::ignore = _reconnectTimer.cancel();

	const bool hasLink{_isConnected && _link && _socket.is_open()};
	_isConnected = false;

	if (!hasLink)
	{
		CloseSocket();
		if (onClosed)
		{
			onClosed();
		}

		return;
	}

	CommandBatch farewell;
	farewell.commands.emplace_back(Disconnect{.reason = reason});
	std::ignore = _link->SendReliable(network::Serialize(farewell));

	const auto now{Clock::now()};
	Transmit(now);
	_onClosed = std::move(onClosed);
	_closeDeadline = now + DatagramLink::kFarewellLinger;
}

void Client::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &Client::OnNetworkEndFrame));
	_subs.push_back(_events->AddListener(this, &Client::OnSlotAssigned));
}

void Client::OnSlotAssigned(const PlayerSlotAssignedEvent& event)
{
	_inputSubs = BindClientInput(_replicationOut, *_events, event.slot);
}

//NOTE: nothing is banked without a link - keys pressed while reconnecting would reach the new one as a burst
void Client::OnNetworkEndFrame(const NetworkEndFrameEvent&)
{
	const auto frame{_replicationOut.TakeFrame()};
	if (!frame)
	{
		return;
	}

	boost::asio::post(_socket.get_executor(), [this, frame]
	{
		if (!_isConnected || !_link)
		{
			return;
		}

		std::ignore = _link->SendFrame(*frame);
		Transmit(Clock::now());
	});
}

void Client::OnDisconnect(const Disconnect& command)
{
	const auto reason{command.reason};

	//NOTE: on the network thread, not in the queued lambda - the EOF arrives well before the game
	//thread drains the queue, and HandleDisconnect must already know why
	//NOTE: giving up is what makes a kick stick - dialling back would take the seat again
	_isLinkUnrecoverable = reason == DisconnectReason::ProtocolError || reason == DisconnectReason::Kicked;
	_isWaitingForSeat = reason == DisconnectReason::ServerFull;

	_commandQueue.Enqueue([this, reason]()
	{
		_events->EmitEvent(ClientInDisconnectEvent{.reason = reason});
	});

	//NOTE: the goodbye is the end of this link - there is no closing socket to report it a second time
	HandleDisconnect();
}
}//namespace network::commands
