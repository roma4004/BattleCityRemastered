#include "components/input/InputProviderForPlayerOneNet.h"
#include "components/EventSystem.h"
#include "components/events/InputEvents.h"

InputProviderForPlayerOneNet::InputProviderForPlayerOneNet(const std::shared_ptr<EventSystem>& events)
	: _events{events} {}

void InputProviderForPlayerOneNet::Subscribe()
{
	_subs.push_back(_events->AddListener(Key(std::string{"P1"}), this, &InputProviderForPlayerOneNet::OnMoveUp));
	_subs.push_back(_events->AddListener(Key(std::string{"P1"}), this, &InputProviderForPlayerOneNet::OnMoveLeft));
	_subs.push_back(_events->AddListener(Key(std::string{"P1"}), this, &InputProviderForPlayerOneNet::OnMoveDown));
	_subs.push_back(_events->AddListener(Key(std::string{"P1"}), this, &InputProviderForPlayerOneNet::OnMoveRight));
	_subs.push_back(_events->AddListener(Key(std::string{"P1"}), this, &InputProviderForPlayerOneNet::OnFire));

	_subs.push_back(_events->AddListener(this, &InputProviderForPlayerOneNet::OnPauseReleased));
}

void InputProviderForPlayerOneNet::OnMoveUp(const ServerInMoveUpEvent& event) { _playerKeys.up = event.isPressed; }

void InputProviderForPlayerOneNet::OnMoveLeft(const ServerInMoveLeftEvent& event)
{
	_playerKeys.left = event.isPressed;
}

void InputProviderForPlayerOneNet::OnMoveDown(const ServerInMoveDownEvent& event)
{
	_playerKeys.down = event.isPressed;
}

void InputProviderForPlayerOneNet::OnMoveRight(const ServerInMoveRightEvent& event)
{
	_playerKeys.right = event.isPressed;
}

void InputProviderForPlayerOneNet::OnFire(const ServerInFireEvent& event) { _playerKeys.shot = event.isPressed; }

void InputProviderForPlayerOneNet::OnPauseReleased(const ServerInPauseReleasedEvent&) const
{
	_events->EmitEvent(PauseReleasedEvent{});
}

void InputProviderForPlayerOneNet::Enable()
{
	Subscribe();
}

void InputProviderForPlayerOneNet::Disable() const
{
	_subs.clear();
}
