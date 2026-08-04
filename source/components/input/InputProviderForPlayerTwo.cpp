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
	const std::string tag{"P2"};
	_events->AddListener("Move_Up", tag, _name, [&btn = _playerKeys](const bool isPressed) { btn.up = isPressed; });
	_events->AddListener("Move_Left", tag, _name, [&btn = _playerKeys](const bool isPressed) { btn.left = isPressed; });
	_events->AddListener("Move_Down", tag, _name, [&btn = _playerKeys](const bool isPressed) { btn.down = isPressed; });
	_events->AddListener("Move_Right", tag, _name, [&btn = _playerKeys](const bool isPressed) { btn.right = isPressed; });
	_events->AddListener("Fire", tag, _name, [&btn = _playerKeys](const bool isPressed) { btn.shot = isPressed; });
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
