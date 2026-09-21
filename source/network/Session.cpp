#include "network/Session.h"
#include "enums/InputChannel.h"
#include "enums/InputSignal.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "network/DatagramLink.h"
#include "network/Serializer.h"
#include "network/WireFrame.h"
#include "network/commands/CommandBatch.h"
#include "utils/Log.h"
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace network::commands
{
namespace
{
//NOTE: a switch, not a table - a signal added to the enum stops compiling here instead of slipping past
//into a runtime log, and the seat's key needs no std::function to travel four lines
void EmitInput(EventSystem& events, const PlayerSlot slot, const InputSignal signal, const bool isPressed)
{
	switch (signal)
	{
		case InputSignal::MoveUp:
			events.EmitEvent(Key(RemoteInput(slot)), MoveUpEvent{.isPressed = isPressed});
			return;
		case InputSignal::MoveDown:
			events.EmitEvent(Key(RemoteInput(slot)), MoveDownEvent{.isPressed = isPressed});
			return;
		case InputSignal::MoveLeft:
			events.EmitEvent(Key(RemoteInput(slot)), MoveLeftEvent{.isPressed = isPressed});
			return;
		case InputSignal::MoveRight:
			events.EmitEvent(Key(RemoteInput(slot)), MoveRightEvent{.isPressed = isPressed});
			return;
		case InputSignal::Fire:
			events.EmitEvent(Key(RemoteInput(slot)), FireEvent{.isPressed = isPressed});
			return;
		//NOTE: the state the client asked for, not a toggle - two clients toggling in turn would cancel
		//each other out, and the authority has no pause of its own to flip
		case InputSignal::PauseRequest:
			events.EmitEvent(SetPauseEvent{.isPaused = isPressed});
			return;
		//NOTE: the host writes this one, it never reads it
		case InputSignal::PauseStatus:
			return;
	}

	Log::Error("Session: unhandled input signal " + std::to_string(static_cast<int>(signal)));
}
}// namespace

Session::Session(udp::endpoint endpoint, const std::uint32_t connectionId, const std::shared_ptr<EventSystem>& events,
				 const PlayerSlot slot, const DatagramLink::Clock::time_point now)
	: PeerLink("Session", events)
	, _endpoint{std::move(endpoint)}
	, _link{connectionId, now}
	, _slot{slot} {}

void Session::OnCommand(const AnyCommand& command)
{
	std::visit([this](const auto& alternative) { Handle(alternative); }, command);
}

//NOTE: the first thing this client hears - everything it sends afterwards is read as that seat's
void Session::Start()
{
	CommandBatch assignment;
	assignment.commands.emplace_back(SlotAssignment{.slot = _slot});
	std::ignore = _link.SendReliable(network::Serialize(assignment));
}

void Session::Receive(const std::string_view datagram, const DatagramLink::Clock::time_point now)
{
	const auto arrivals{_link.Receive(datagram, now)};
	if (!arrivals || _isPeerGone)
	{
		return;
	}

	if (!Dispatch(*arrivals))
	{
		_isPeerGone = true;
		Shutdown(DisconnectReason::ProtocolError);
	}
}

void Session::Send(const WireFrame& frame)
{
	if (!_link.SendFrame(frame))
	{
		Log::Info("Session: the client fell too far behind, its backlog is dropped for a snapshot");
		_isSnapshotOwed.store(true, std::memory_order_release);
	}
}

std::vector<std::string> Session::TakeDatagrams(const DatagramLink::Clock::time_point now)
{
	return _link.TakeDatagrams(now);
}

void Session::LoseIfSilent(const DatagramLink::Clock::time_point now)
{
	if (_link.IsSilent(now))
	{
		Lose();
	}
}

void Session::Supersede() { Lose(); }

void Session::Lose()
{
	if (_isPeerGone)
	{
		return;
	}

	_isPeerGone = true;
	_commandQueue.Enqueue([this]
	{
		ReleaseSeatKeys();
		_events->EmitEvent(ServerClientLostEvent{.slot = _slot});
	});

	MarkFinished();
}

void Session::Shutdown(const DisconnectReason reason)
{
	CommandBatch farewell;
	farewell.commands.emplace_back(Disconnect{.reason = reason});
	std::ignore = _link.SendReliable(network::Serialize(farewell));

	MarkFinished();
}

//NOTE: the goodbye goes out on the next tick, the seat is given up here - a kicked client is not coming back
void Session::Kick()
{
	Shutdown(DisconnectReason::Kicked);
	Lose();
}

void Session::Handle(const Disconnect& command)
{
	_isPeerGone = true;

	_commandQueue.Enqueue([this, reason = command.reason]()
	{
		ReleaseSeatKeys();
		_events->EmitEvent(ServerInDisconnectEvent{.reason = reason, .slot = _slot});
	});

	MarkFinished();
}

void Session::Handle(const SignalEvent& command)
{
	_commandQueue.Enqueue([this, signal = command.signal]()
	{
		switch (signal)
		{
			case ClientSignal::ReadyToPlay:
				//NOTE: here, on the game thread - owed any earlier, it could be paid before the ready it answers
				_isSnapshotOwed.store(true, std::memory_order_release);
				_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = _slot});
				break;
			case ClientSignal::RestartMatch:
				_events->EmitEvent(ServerInRestartRequestedEvent{});
				break;
		}
	});
}

void Session::Handle(const KeyStateChange& command)
{
	_commandQueue.Enqueue([this, cmd = command] { EmitInput(*_events, _slot, cmd.action, cmd.isPressed); });
}

void Session::ReleaseSeatKeys() const
{
	for (const InputSignal key: {InputSignal::MoveUp, InputSignal::MoveDown, InputSignal::MoveLeft,
								 InputSignal::MoveRight, InputSignal::Fire})
	{
		EmitInput(*_events, _slot, key, false);
	}
}
}//namespace network::commands
