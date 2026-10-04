#pragma once

#include "components/EventSystem.h"
#include <memory>
#include <vector>

struct SetPauseEvent;
struct MatchHoldChangedEvent;
class EventSystem;

//NOTE: in the game the pause belongs to the menu, and there is no menu on the server - so a console command and a
//client's request would reach nothing. Everything downstream waits on PauseStatusEvent, this is what says it
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
	//NOTE: a newcomer catches up, or the players decide about one who left - nobody's unpause lets the match go
	bool _isHeld{};
};
