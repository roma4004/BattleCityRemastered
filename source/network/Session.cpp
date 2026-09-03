#include "network/Session.h"
#include "enums/InputChannel.h"
#include "enums/PlayerTag.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "network/Serializer.h"
#include "utils/Log.h"
#include <string>
#include <variant>

namespace network::commands
{
Session::Session(tcp::socket sock, const std::shared_ptr<EventSystem>& events)
	: PeerLink(std::move(sock), "Session", events) {}

void Session::OnCommand(const AnyCommand& command)
{
	std::visit([this](const auto& alternative) { Handle(alternative); }, command);
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

void Session::Handle(const Disconnect& command)
{
	_isPeerGone = true;

	_commandQueue.Enqueue([this, reason = command.reason]()
	{
		_channel->Close();
		_events->EmitEvent(ServerInDisconnectEvent{.reason = reason});
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

void Session::Handle(const SignalEvent& command)
{
	_commandQueue.Enqueue([this, signal = command.signal]()
	{
		switch (signal)
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
		 [](EventSystem& events, const PlayerSlot slot, const bool pressed)
		 {
			 events.EmitEvent(Key(RemoteInput(slot)), MoveUpEvent{.isPressed = pressed});
		 }},
		{InputSignal::MoveDown,
		 [](EventSystem& events, const PlayerSlot slot, const bool pressed)
		 {
			 events.EmitEvent(Key(RemoteInput(slot)), MoveDownEvent{.isPressed = pressed});
		 }},
		{InputSignal::MoveLeft,
		 [](EventSystem& events, const PlayerSlot slot, const bool pressed)
		 {
			 events.EmitEvent(Key(RemoteInput(slot)), MoveLeftEvent{.isPressed = pressed});
		 }},
		{InputSignal::MoveRight,
		 [](EventSystem& events, const PlayerSlot slot, const bool pressed)
		 {
			 events.EmitEvent(Key(RemoteInput(slot)), MoveRightEvent{.isPressed = pressed});
		 }},
		{InputSignal::Fire,
		 [](EventSystem& events, const PlayerSlot slot, const bool pressed)
		 {
			 events.EmitEvent(Key(RemoteInput(slot)), FireEvent{.isPressed = pressed});
		 }},
		{InputSignal::PauseReleased,
		 [](EventSystem& events, PlayerSlot, const bool)
		 {
			 events.EmitEvent(PauseReleasedEvent{});
		 }},
};

void Session::Handle(const KeyStateChange& command)
{
	_commandQueue.Enqueue([this, cmd = command]()
	{
		const auto it = kInputEmitters.find(cmd.action);
		if (it == kInputEmitters.end())
		{
			Log::Error("Session: unhandled input signal "
					   + std::to_string(static_cast<int>(cmd.action)));
			return;
		}

		const auto& emit = it->second;
		const auto& playerSlot = cmd.tag == PlayerTag::P1 ? PlayerSlot::P1 : PlayerSlot::P2;
		emit(*_events, playerSlot, cmd.isPressed);
	});
}

void Session::DoWrite(std::shared_ptr<const std::string> message) { _channel->Send(std::move(message)); }
}//namespace network::commands
