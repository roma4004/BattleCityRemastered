#include "network/Client.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "enums/ClientSignal.h"
#include "network/ReplicationBindings.h"
#include "utils/Log.h"
#include <string>
#include <tuple>
#include <utility>
#include <variant>

namespace network::commands
{
Client::Client(boost::asio::io_context& ioContext, std::string host, const uint16_t port,
			   const std::shared_ptr<EventSystem>& events)
	: PeerLink(tcp::socket(boost::asio::make_strand(ioContext)), "Client", events)
	, _reconnectTimer(_channel->Socket().get_executor())
	, _endpoint{tcp::endpoint(boost::asio::ip::make_address(host), port)}
	, _replicationIn{events, _commandQueue}
	, _replicationOut{events}
{
	BindClientReplication(_replicationOut);
	Subscribe();

	TryConnect();
}

void Client::OnCommand(const AnyCommand& command)
{
	//NOTE: the goodbye is peeled off rather than left to the applier - it has to be read on this, the
	//network thread, while everything else is a game fact and belongs on the game one
	if (const auto* goodbye = std::get_if<Disconnect>(&command))
	{
		OnDisconnect(*goodbye);
		return;
	}

	_replicationIn.Apply(command);
}

void Client::TryConnect()
{
	auto& socket = _channel->Socket();
	_channel->CloseForReconnect();
	socket.open(_endpoint.protocol());
	socket.async_connect(_endpoint, [this](const boost::system::error_code& ec)
	{
		if (!ec)
		{
			Log::Info("client connected");
			_reconnectAttempts = 0;
			_reconnectAbandoned = false;
			_isConnected = true;
			_commandQueue.Enqueue([this] { _events->EmitEvent(ClientConnectedToHostEvent{}); });
			this->StartReading();
			_channel->SetWriteEnabled(true);
			_replicationOut.Publish(SignalEvent{.signal = ClientSignal::ReadyToPlay});
		}
		else
		{
			++_reconnectAttempts;
			Log::Error("Client connect failed (attempt " + std::to_string(_reconnectAttempts) + "/"
					   + std::to_string(kMaxReconnectAttempts) + "): " + ec.message());

			ScheduleReconnect();
		}
	});
}

void Client::ScheduleReconnect()
{
	if (_reconnectPending || _isShuttingDown)
	{
		return;
	}

	if (_isLinkUnrecoverable || _reconnectAttempts >= kMaxReconnectAttempts)
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
	const std::weak_ptr<Client> weakSelf = weak_from_this();
	_reconnectTimer.expires_after(std::chrono::milliseconds(kReconnectDelayMs));
	_reconnectTimer.async_wait([weakSelf](const boost::system::error_code& timerEc)
	{
		const auto self = weakSelf.lock();
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
	_channel->SetWriteEnabled(false);

	if (_isLinkUnrecoverable)
	{
		_channel->Close();
	}
	else
	{
		_channel->CloseForReconnect();
		_reconnectAttempts = 0;//NOTE: a drop starts a fresh budget, it is not a failed connect attempt
	}

	ScheduleReconnect();
}

//NOTE: TCP hands bytes over intact or not at all, so an unreadable frame is a protocol disagreement
//and the next one fails the same way - the link ends here instead of retrying
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

Client::~Client()
{
	Shutdown();
}

void Client::Shutdown()
{
	_isShuttingDown = true;//NOTE: before cancelling - handlers must not read the cancel as a drop

	std::ignore = _reconnectTimer.cancel();//NOTE: no-throw, so ~Client is safe without a catch-all
	_channel->Close();
}

void Client::Shutdown(const DisconnectReason reason, std::function<void()> onClosed)
{
	_isShuttingDown = true;
	std::ignore = _reconnectTimer.cancel();

	const bool hasLink = _isConnected;
	_isConnected = false;

	CloseWithFarewell(hasLink, reason, std::move(onClosed));
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

void Client::OnNetworkEndFrame(const NetworkEndFrameEvent&)
{
	if (auto frame = _replicationOut.TakeFrame())
	{
		_channel->Send(std::move(frame));
	}
}

void Client::StartReading()
{
	//NOTE: weak, not shared - the channel outlives nothing here, but it *stores* these callbacks,
	//so capturing a shared_ptr would close the loop Client -> channel -> callback -> Client
	const std::weak_ptr<Client> weakSelf = weak_from_this();
	_channel->SetHandlers(
			[weakSelf](const std::string& frame)
			{
				if (const auto self = weakSelf.lock(); self && !self->DispatchFrame(frame))
				{
					self->HandleProtocolError();
				}
			},
			[weakSelf]
			{
				if (const auto self = weakSelf.lock())
				{
					self->HandleDisconnect();
				}
			});

	_channel->StartReading();
}

void Client::OnDisconnect(const Disconnect& command)
{
	const auto reason = command.reason;

	//NOTE: on the network thread, not in the queued lambda - the EOF arrives well before the game
	//thread drains the queue, and HandleDisconnect must already know why
	_isLinkUnrecoverable = reason == DisconnectReason::ProtocolError;

	_commandQueue.Enqueue([this, reason]()
	{
		_events->EmitEvent(ClientInDisconnectEvent{.reason = reason});
	});
}

}//namespace network::commands
