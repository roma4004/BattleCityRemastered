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
	const std::string tag{"P1"};
	_events->AddListener("Move_Up", tag, _name, [&btn = _playerKeys](const bool isPressed) { btn.up = isPressed; });
	_events->AddListener("Move_Left", tag, _name, [&btn = _playerKeys](const bool isPressed) { btn.left = isPressed; });
	_events->AddListener("Move_Down", tag, _name, [&btn = _playerKeys](const bool isPressed) { btn.down = isPressed; });
	_events->AddListener("Move_Right", tag, _name, [&btn = _playerKeys](const bool isPressed) { btn.right = isPressed; });
	_events->AddListener("Fire", tag, _name, [&btn = _playerKeys](const bool isPressed) { btn.shot = isPressed; });
}

void InputProviderForPlayerOne::Unsubscribe() const { _events->RemoveAllListeners(_name); }

void InputProviderForPlayerOne::Enable()
{
	Subscribe();
}

void InputProviderForPlayerOne::Disable() const
{
	Unsubscribe();
}
