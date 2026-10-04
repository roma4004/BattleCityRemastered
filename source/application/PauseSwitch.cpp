#include "application/PauseSwitch.h"
#include "components/EventSystem.h"
#include "components/events/InputEvents.h"
#include <memory>

PauseSwitch::PauseSwitch(const std::shared_ptr<EventSystem>& events)
	: _events{events}
{
	_subs.push_back(_events->AddListener(this, &PauseSwitch::OnSetPause));
	_subs.push_back(_events->AddListener(this, &PauseSwitch::OnHoldChanged));
}

//NOTE: an ask the hold overrules is answered anyway - the client that let go of the pause has already let go of
//its own, and only the answer puts it back
void PauseSwitch::OnSetPause(const SetPauseEvent& event)
{
	_isAskedFor = event.isPaused;
	const bool isOverruled{!_isAskedFor && _isHeld};
	Announce(isOverruled);
}

void PauseSwitch::OnHoldChanged(const MatchHoldChangedEvent& event)
{
	_isHeld = event.isHeld;
	Announce(false);
}

void PauseSwitch::Announce(const bool isForced)
{
	const bool isPaused{_isAskedFor || _isHeld};
	if (isPaused == _isPaused && !isForced)
	{
		return;
	}

	_isPaused = isPaused;
	_events->EmitEvent(PauseStatusEvent{.isPaused = _isPaused});
}
