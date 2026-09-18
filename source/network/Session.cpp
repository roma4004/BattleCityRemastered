#include "network/Session.h"
#include "enums/InputChannel.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "network/Serializer.h"
#include "utils/Log.h"
#include <string>
#include <utility>
#include <variant>

namespace network::commands
{
Session::Session(tcp::socket sock, const std::shared_ptr<EventSystem>& events, const PlayerSlot slot,
				 std::string address)
	: PeerLink(std::move(sock), "Session", events)
	, _slot{slot}
	, _address{std::move(address)} {}

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

void Session::Kick()
{
	Shutdown(DisconnectReason::Kicked, [weakSelf = weak_from_this()]
	{
		if (const auto self{weakSelf.lock()})
		{
			self->LoseLink();
		}
	});
}

void Session::LoseLink()
{
	_channel->Close();

	if (!_isPeerGone)
	{
		_isPeerGone = true;
		_commandQueue.Enqueue([self = shared_from_this()] { self->_events->EmitEvent(ServerClientLostEvent{}); });
	}

	MarkFinished();
}

void Session::Handle(const Disconnect& command)
{
	_isPeerGone = true;

	_commandQueue.Enqueue([this, reason = command.reason]()
	{
		_channel->Close();
		_events->EmitEvent(ServerInDisconnectEvent{.reason = reason});
	});

	MarkFinished();
}

void Session::Start()
{
	//NOTE: weak, not shared - the channel stores these callbacks, so a shared_ptr would close the
	//loop Session -> channel -> callback -> Session
	const std::weak_ptr<Session> weakSelf{weak_from_this()};
	_channel->SetHandlers(
			[weakSelf](const std::string& frame)
			{
				if (const auto self{weakSelf.lock()}; self && !self->DispatchFrame(frame))
				{
					//NOTE: finished only once the goodbye is out - CloseAfterFlush still has it to write
					self->Shutdown(DisconnectReason::ProtocolError, [self] { self->MarkFinished(); });
				}
			},
			[weakSelf]
			{
				if (const auto self{weakSelf.lock()})
				{
					self->LoseLink();
				}
			});

	_channel->StartReading();

	//NOTE: the first thing this client hears - everything it sends afterwards is read as that seat's
	CommandBatch assignment;
	assignment.commands.emplace_back(SlotAssignment{.slot = _slot});
	SendBatch(assignment);
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
			case ClientSignal::RestartMatch:
				_events->EmitEvent(ServerInRestartRequestedEvent{});
				break;
		}
	});
}

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
		const auto it{kInputEmitters.find(cmd.action)};
		if (it == kInputEmitters.end())
		{
			Log::Error("Session: unhandled input signal "
					   + std::to_string(static_cast<int>(cmd.action)));
			return;
		}

		const auto& emit{it->second};
		emit(*_events, _slot, cmd.isPressed);
	});
}

void Session::DoWrite(std::shared_ptr<const std::string> message) { _channel->Send(std::move(message)); }
}//namespace network::commands
