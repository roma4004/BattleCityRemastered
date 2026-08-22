#include "components/input/InputProviderForPlayerTwo.h"
#include "components/EventSystem.h"
#include "components/events/InputEvents.h"

InputProviderForPlayerTwo::InputProviderForPlayerTwo(const std::shared_ptr<EventSystem>& events)
	: _events{events} {}

void InputProviderForPlayerTwo::Subscribe()
{
	_subs.push_back(_events->AddListener(Key(std::string{"P2"}), this, &InputProviderForPlayerTwo::OnMoveUp));
	_subs.push_back(_events->AddListener(Key(std::string{"P2"}), this, &InputProviderForPlayerTwo::OnMoveLeft));
	_subs.push_back(_events->AddListener(Key(std::string{"P2"}), this, &InputProviderForPlayerTwo::OnMoveDown));
	_subs.push_back(_events->AddListener(Key(std::string{"P2"}), this, &InputProviderForPlayerTwo::OnMoveRight));
	_subs.push_back(_events->AddListener(Key(std::string{"P2"}), this, &InputProviderForPlayerTwo::OnFire));
}

void InputProviderForPlayerTwo::OnMoveUp(const MoveUpEvent& event) { _playerKeys.up = event.isPressed; }
void InputProviderForPlayerTwo::OnMoveLeft(const MoveLeftEvent& event) { _playerKeys.left = event.isPressed; }
void InputProviderForPlayerTwo::OnMoveDown(const MoveDownEvent& event) { _playerKeys.down = event.isPressed; }
void InputProviderForPlayerTwo::OnMoveRight(const MoveRightEvent& event) { _playerKeys.right = event.isPressed; }
void InputProviderForPlayerTwo::OnFire(const FireEvent& event) { _playerKeys.shot = event.isPressed; }

void InputProviderForPlayerTwo::Enable()
{
	Subscribe();
}

void InputProviderForPlayerTwo::Disable()
{
	_subs.clear();
}
