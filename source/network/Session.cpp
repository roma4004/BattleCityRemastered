#include "network/Session.h"
#include "enums/PlayerTag.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "network/Serializer.h"
#include "utils/Log.h"
#include <string>

namespace network::commands
{
Session::Session(tcp::socket sock, const std::shared_ptr<EventSystem>& events)
	: PeerLink(std::move(sock), "Session", events)
{
	RegisterCommandHandlers();
}

void Session::RegisterCommandHandlers()
{
	_dispatcher.RegisterAll({
			{CommandType::SIGNAL_EVENT, [this](const AnyCommand& cmd) { OnSignalEvent(cmd); }},
			{CommandType::KEY_STATE_CHANGE, [this](const AnyCommand& cmd) { OnKeyStateChange(cmd); }},
			{CommandType::DISCONNECT, [this](const AnyCommand& cmd) { OnDisconnect(cmd); }},
	});
}

Session::~Session()
{
	Shutdown();
}

void Session::Shutdown() { _channel->Close(); }

void Session::Shutdown(const DisconnectReason reason, std::function<void()> onClosed)
{
	CloseWithFarewell(_channel->IsOpen(), reason, std::move(onClosed));
}

void Session::OnDisconnect(const AnyCommand& command)
{
	_isPeerGone = true;

	_commandQueue.Enqueue([this, cmd = std::get<Disconnect>(command)]()
	{
		_channel->Close();
		_events->EmitEvent(ServerInDisconnectEvent{.reason = cmd.reason});
	});
}

void Session::Start()
{
	//NOTE: weak, not shared - the channel stores these callbacks, so a shared_ptr would close the
	//loop Session -> channel -> callback -> Session
	const std::weak_ptr<Session> weakSelf = weak_from_this();
	_channel->SetHandlers(
			[weakSelf](const std::string& frame)
			{
				if (const auto self = weakSelf.lock(); self && !self->DispatchFrame(frame))
				{
					self->Shutdown(DisconnectReason::ProtocolError, nullptr);
				}
			},
			[weakSelf]
			{
				const auto self = weakSelf.lock();
				if (!self)
				{
					return;
				}

				self->_channel->Close();

				if (!self->_isPeerGone)
				{
					self->_isPeerGone = true;
					self->_commandQueue.Enqueue([self]
					{
						self->_events->EmitEvent(ServerClientLostEvent{});
					});
				}
			});

	_channel->StartReading();
}

void Session::OnSignalEvent(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<SignalEvent>(command)]()
	{
		switch (cmd.signal)
		{
			case ClientSignal::ReadyToPlay:
				_events->EmitEvent(ServerInClientReadyToStartGameEvent{});
				break;
		}
	});
}

//NOTE: dispatch table instead of a switch, same shape as the command handlers above
const std::unordered_map<InputSignal, Session::InputEmitter> Session::kInputEmitters{
		{InputSignal::MoveUp,
		 [](EventSystem& events, const std::string& tag, const bool pressed)
		 { events.EmitEvent(Key(tag), ServerInMoveUpEvent{.isPressed = pressed}); }},
		{InputSignal::MoveDown,
		 [](EventSystem& events, const std::string& tag, const bool pressed)
		 { events.EmitEvent(Key(tag), ServerInMoveDownEvent{.isPressed = pressed}); }},
		{InputSignal::MoveLeft,
		 [](EventSystem& events, const std::string& tag, const bool pressed)
		 { events.EmitEvent(Key(tag), ServerInMoveLeftEvent{.isPressed = pressed}); }},
		{InputSignal::MoveRight,
		 [](EventSystem& events, const std::string& tag, const bool pressed)
		 { events.EmitEvent(Key(tag), ServerInMoveRightEvent{.isPressed = pressed}); }},
		{InputSignal::Fire,
		 [](EventSystem& events, const std::string& tag, const bool pressed)
		 { events.EmitEvent(Key(tag), ServerInFireEvent{.isPressed = pressed}); }},
		{InputSignal::PauseReleased,
		 [](EventSystem& events, const std::string&, const bool)
		 { events.EmitEvent(PauseReleasedEvent{}); }},
};

void Session::OnKeyStateChange(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<KeyStateChange>(command)]()//TODO: validate each command, security risk
	{
		const auto it = kInputEmitters.find(cmd.action);
		if (it == kInputEmitters.end())
		{
			Log::Error("Session::OnKeyStateChange: unhandled input signal "
									  + std::to_string(static_cast<int>(cmd.action)));
			return;
		}

		//NOTE: the local bus is still keyed by the "P1"/"P2" string, only the wire is typed
		it->second(*_events, cmd.tag == PlayerTag::P1 ? "P1" : "P2", cmd.isPressed);
	});
}

void Session::DoWrite(std::shared_ptr<const std::string> message) { _channel->Send(std::move(message)); }
}//namespace network::commands
