#include "components/input/InputProviderForPlayerOne.h"
#include "components/EventSystem.h"
#include "components/events/InputEvents.h"

InputProviderForPlayerOne::InputProviderForPlayerOne(const std::shared_ptr<EventSystem>& events)
	: _events{events} {}

void InputProviderForPlayerOne::Subscribe()
{
	_subs.push_back(_events->AddListener(Key(std::string{"P1"}), this, &InputProviderForPlayerOne::OnMoveUp));
	_subs.push_back(_events->AddListener(Key(std::string{"P1"}), this, &InputProviderForPlayerOne::OnMoveLeft));
	_subs.push_back(_events->AddListener(Key(std::string{"P1"}), this, &InputProviderForPlayerOne::OnMoveDown));
	_subs.push_back(_events->AddListener(Key(std::string{"P1"}), this, &InputProviderForPlayerOne::OnMoveRight));
	_subs.push_back(_events->AddListener(Key(std::string{"P1"}), this, &InputProviderForPlayerOne::OnFire));
}

void InputProviderForPlayerOne::OnMoveUp(const MoveUpEvent& event) { _playerKeys.up = event.isPressed; }
void InputProviderForPlayerOne::OnMoveLeft(const MoveLeftEvent& event) { _playerKeys.left = event.isPressed; }
void InputProviderForPlayerOne::OnMoveDown(const MoveDownEvent& event) { _playerKeys.down = event.isPressed; }
void InputProviderForPlayerOne::OnMoveRight(const MoveRightEvent& event) { _playerKeys.right = event.isPressed; }
void InputProviderForPlayerOne::OnFire(const FireEvent& event) { _playerKeys.shot = event.isPressed; }

void InputProviderForPlayerOne::Enable()
{
	Subscribe();
}

void InputProviderForPlayerOne::Disable()
{
	_subs.clear();
}
