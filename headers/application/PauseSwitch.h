#pragma once

#include "components/EventSystem.h"
#include <memory>
#include <vector>

struct SetPauseEvent;
struct MatchHoldChangedEvent;
class EventSystem;

//NOTE: the server's pause, which has no menu - takes console and client requests and emits PauseStatusEvent
class PauseSwitch final
{
public:
	explicit PauseSwitch(const std::shared_ptr<EventSystem>& events);

private:
	void OnSetPause(const SetPauseEvent& event);
	void OnHoldChanged(const MatchHoldChangedEvent& event);
	void Announce(bool isForced);

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	bool _isPaused{};
	//NOTE: what a player or the console asked for last
	bool _isAskedFor{};
	//NOTE: from MatchHoldChangedEvent - the match stays paused, an unpause asked meanwhile gets the pause back
	bool _isHeld{};
};
