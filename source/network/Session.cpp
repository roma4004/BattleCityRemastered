#include "network/Session.h"
#include "enums/Absence.h"
#include "enums/DisconnectReason.h"
#include "enums/InputChannel.h"
#include "enums/InputSignal.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/MatchSettings.h"
#include "network/DatagramLink.h"
#include "network/Serializer.h"
#include "network/WireFrame.h"
#include "network/commands/CommandBatch.h"
#include "network/commands/SlotAssignment.h"
#include "utils/Log.h"
#include <cstdint>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <variant>
#include <vector>

namespace network
{
namespace
{
//NOTE: a switch, not a table - a new signal fails to compile here instead of slipping into a runtime log
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
		//NOTE: the asked state, not a toggle - two clients toggling would cancel out; the authority has no own pause
		case InputSignal::PauseRequest:
			events.EmitEvent(SetPauseEvent{.isPaused = isPressed});
			return;
		//NOTE: the host writes this one, it never reads it
		case InputSignal::PauseStatus:
			return;
	}

	Log::Error("Session: unhandled input signal " + std::to_string(static_cast<int>(signal)));
}

//NOTE: a switch for the same reason - a signal added to the enum has to say whether it takes the tank from a bot
[[nodiscard]] bool IsTankCommand(const InputSignal signal)
{
	switch (signal)
	{
		case InputSignal::MoveUp:
		case InputSignal::MoveDown:
		case InputSignal::MoveLeft:
		case InputSignal::MoveRight:
		case InputSignal::Fire:
			return true;
		case InputSignal::PauseRequest:
		case InputSignal::PauseStatus:
			return false;
	}

	return false;
}
}// namespace

Session::Session(udp::endpoint endpoint, const std::uint32_t connectionId, const std::shared_ptr<EventSystem>& events,
				 const PlayerSlot slot, MatchSettings match, const DatagramLink::Clock::time_point now)
	: PeerLink("Session", events)
	, _endpoint{std::move(endpoint)}
	, _link{connectionId, now}
	, _slot{slot}
	, _match{std::move(match)} {}

void Session::OnCommand(const commands::AnyCommand& command)
{
	std::visit([this](const auto& alternative) { Handle(alternative); }, command);
}

//NOTE: the first thing this client hears - everything it sends afterwards is read as that seat's
void Session::Start()
{
	commands::CommandBatch assignment;
	assignment.commands.emplace_back(commands::SlotAssignment{.slot = _slot, .match = _match});
	std::ignore = _link.SendReliable(network::Serialize(assignment));

	_commandQueue.Enqueue([this] { _events->EmitEvent(ServerClientSeatedEvent{.slot = _slot}); });
}

void Session::Receive(const std::string_view datagram, const DatagramLink::Clock::time_point now)
{
	const auto arrivals{_link.Receive(datagram, now)};
	if (!arrivals || _isPeerGone)
	{
		return;
	}

	//NOTE: the seat goes as with a kick - left held, the lobby would wait for this client and its keys stay pressed
	if (!Dispatch(*arrivals))
	{
		Lose();
		Shutdown(DisconnectReason::ProtocolError);
	}
}

void Session::Send(const WireFrame& frame)
{
	//NOTE: behind the goodbye - the client drops the link on it, and a frame it never acks keeps this one from draining
	if (IsFinished())
	{
		return;
	}

	if (!_link.SendFrame(frame))
	{
		Log::Info("Session: the client fell too far behind, its backlog is dropped for a snapshot");
		OweSnapshot();
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

void Session::UpdateWoundDown(const DatagramLink::Clock::time_point now)
{
	const bool isWoundDown{IsFinished() && (_link.IsDrained() || _link.IsSilent(now))};
	_isWoundDown.store(isWoundDown, std::memory_order_release);
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
	_link.SendGoodbye(SerializeGoodbye(reason));

	MarkFinished();
}

//NOTE: the goodbye goes out on the next tick, the seat is given up here - a kicked client is not coming back
void Session::Kick()
{
	//NOTE: kicked twice, or gone meanwhile - a second goodbye would wait for an ack nobody sends
	if (IsFinished())
	{
		return;
	}

	Lose();
	Shutdown(DisconnectReason::Kicked);
}

void Session::Handle(const commands::Disconnect& command)
{
	_isPeerGone = true;

	_commandQueue.Enqueue([this, reason = command.reason]()
	{
		ReleaseSeatKeys();
		_events->EmitEvent(ServerInDisconnectEvent{.reason = reason, .slot = _slot});
	});

	MarkFinished();
}

void Session::Handle(const commands::SignalEvent& command)
{
	_commandQueue.Enqueue([this, signal = command.signal]()
	{
		switch (signal)
		{
			case ClientSignal::ReadyToPlay:
				//NOTE: here, on the game thread - set earlier, the snapshot could go out before the ready it answers
				OweSnapshot();
				_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = _slot});
				break;
			case ClientSignal::RestartMatch:
				_events->EmitEvent(ServerInRestartRequestedEvent{});
				break;
			case ClientSignal::NextLevel:
				_events->EmitEvent(NextLevelRequestedEvent{});
				break;
			case ClientSignal::WorldSynced:
				_events->EmitEvent(ServerClientSyncedEvent{.slot = _slot});
				break;
			case ClientSignal::AbsenceContinue:
				_events->EmitEvent(AbsenceChosenEvent{.choice = AbsenceChoice::Continue});
				break;
			case ClientSignal::AbsenceBot:
				_events->EmitEvent(AbsenceChosenEvent{.choice = AbsenceChoice::Bot});
				break;
		}
	});
}

//NOTE: the tank is taken before the key goes on, so the key that takes it drives it as well
void Session::Handle(const commands::KeyStateChange& command)
{
	_commandQueue.Enqueue([this, cmd = command]
	{
		if (cmd.isPressed && IsTankCommand(cmd.action))
		{
			_events->EmitEvent(ServerInTankCommandEvent{.slot = _slot});
		}

		EmitInput(*_events, _slot, cmd.action, cmd.isPressed);
	});
}

void Session::ReleaseSeatKeys() const
{
	for (const InputSignal key: {InputSignal::MoveUp, InputSignal::MoveDown, InputSignal::MoveLeft,
								 InputSignal::MoveRight, InputSignal::Fire})
	{
		EmitInput(*_events, _slot, key, false);
	}
}
}//namespace network
