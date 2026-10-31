#include "network/Client.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "network/DatagramLink.h"
#include "network/Endpoints.h"
#include "network/ReplicationBindings.h"
#include "network/Serializer.h"
#include "utils/Log.h"
#include "utils/RandUtils.h"
#include <boost/asio/ip/address.hpp>
#include <boost/asio/post.hpp>
#include <cstdint>
#include <random>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>

namespace network
{
Client::Client(boost::asio::io_context& ioContext, const ServerAddress& address,
			   const std::shared_ptr<EventSystem>& events)
	: PeerLink("Client", events)
	, _endpoint{boost::asio::ip::make_address(DialHost(address.host)), address.port}
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

void Client::OnCommand(const commands::AnyCommand& command)
{
	if (const auto* goodbye{std::get_if<commands::Disconnect>(&command)})
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

							  //NOTE: one datagram's error - Windows reports a refusal on the next receive
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

	//NOTE: after the batch - a refusal comes in the answer to the hello and drops the link, so no seat is reported
	if (!isFirstAnswer || !_link || _isShuttingDown)
	{
		return;
	}

	Log::Info("client connected");
	_reconnectAttempts = 0;
	_reconnectAbandoned = false;
	//NOTE: seated now - a host lost later is given up on after the usual attempts, not waited for as a full one
	_isWaitingForSeat = false;
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

	//NOTE: weak - the timer is our member, so a shared capture would keep this Client alive through its own handler
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

//NOTE: an unreadable message arrived whole and in order - a protocol mismatch, so the link ends here
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

	_link->SendGoodbye(SerializeGoodbye(reason));

	const auto now{Clock::now()};
	Transmit(now);
	_onClosed = std::move(onClosed);
	_closeDeadline = now + DatagramLink::kFarewellLinger;
}

void Client::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &Client::OnSendNetworkFrame));
	_subs.push_back(_events->AddListener(this, &Client::OnSlotAssigned));
}

void Client::OnSlotAssigned(const PlayerSlotAssignedEvent& event)
{
	_inputSubs = BindClientInput(_replicationOut, *_events, event.slot);
}

//NOTE: nothing is queued without a link - keys pressed while reconnecting would arrive as a burst
void Client::OnSendNetworkFrame(const SendNetworkFrameEvent&)
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

void Client::OnDisconnect(const commands::Disconnect& command)
{
	const auto reason{command.reason};

	//NOTE: set here, before the queue drains, for HandleDisconnect; a protocol error or a kick stops redialing
	_isLinkUnrecoverable = reason == DisconnectReason::ProtocolError || reason == DisconnectReason::Kicked;
	_isWaitingForSeat = reason == DisconnectReason::ServerFull;

	_commandQueue.Enqueue([this, reason]()
	{
		_events->EmitEvent(ClientInDisconnectEvent{.reason = reason});
	});

	//NOTE: the goodbye is the end of this link - there is no closing socket to report it a second time
	HandleDisconnect();
}
}//namespace network
