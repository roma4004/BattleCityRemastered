#include "components/input/InputProviderForPlayerOne.h"
#include "components/EventSystem.h"

InputProviderForPlayerOne::InputProviderForPlayerOne(const std::shared_ptr<EventSystem>& events)
	: _events{events} {}

InputProviderForPlayerOne::~InputProviderForPlayerOne()
{
	Unsubscribe();
}

void InputProviderForPlayerOne::Subscribe()
{
	_events->AddListener("P1_Move_Up", _name, [&btn = _playerKeys](const bool isPressed) { btn.up = isPressed; });
	_events->AddListener("P1_Move_Left", _name, [&btn = _playerKeys](const bool isPressed) { btn.left = isPressed; });
	_events->AddListener("P1_Move_Down", _name, [&btn = _playerKeys](const bool isPressed) { btn.down = isPressed; });
	_events->AddListener("P1_Move_Right", _name, [&btn = _playerKeys](const bool isPressed) { btn.right = isPressed; });
	_events->AddListener("P1_Fire", _name, [&btn = _playerKeys](const bool isPressed) { btn.shot = isPressed; });
}

void InputProviderForPlayerOne::Unsubscribe() const
{
	_events->RemoveListener("P1_Move_Up", _name);
	_events->RemoveListener("P1_Move_Left", _name);
	_events->RemoveListener("P1_Move_Down", _name);
	_events->RemoveListener("P1_Move_Right", _name);
	_events->RemoveListener("P1_Fire", _name);
}

void InputProviderForPlayerOne::Enable()
{
	Subscribe();
}

void InputProviderForPlayerOne::Disable() const
{
	Unsubscribe();
}
