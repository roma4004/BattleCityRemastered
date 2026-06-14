#include "components/input/InputProviderForPlayerTwo.h"
#include "components/EventSystem.h"

InputProviderForPlayerTwo::InputProviderForPlayerTwo(const std::shared_ptr<EventSystem>& events)
	: _events{events} {}

InputProviderForPlayerTwo::~InputProviderForPlayerTwo()
{
	Unsubscribe();
}

void InputProviderForPlayerTwo::Subscribe()
{
	_events->AddListener("P2_Move_Up", _name, [&btn = _playerKeys](const bool isPressed) { btn.up = isPressed; });
	_events->AddListener("P2_Move_Left", _name, [&btn = _playerKeys](const bool isPressed) { btn.left = isPressed; });
	_events->AddListener("P2_Move_Down", _name, [&btn = _playerKeys](const bool isPressed) { btn.down = isPressed; });
	_events->AddListener("P2_Move_Right", _name, [&btn = _playerKeys](const bool isPressed) { btn.right = isPressed; });
	_events->AddListener("P2_Fire", _name, [&btn = _playerKeys](const bool isPressed) { btn.shot = isPressed; });

}

void InputProviderForPlayerTwo::Unsubscribe() const { _events->RemoveAllListeners(_name); }

void InputProviderForPlayerTwo::Enable()
{
	Subscribe();
}

void InputProviderForPlayerTwo::Disable() const
{
	Unsubscribe();
}
