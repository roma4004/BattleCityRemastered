#include "components/input/InputProviderForPlayerTwo.h"
#include "components/EventSystem.h"

InputProviderForPlayerTwo::InputProviderForPlayerTwo(std::shared_ptr<EventSystem> events)
	: _events{std::move(events)}
{
	// for keybord
	_events->AddListener("ArrowUp_Pressed", _name, [&btn = _playerKeys]() { btn.up = true; });
	_events->AddListener("ArrowUp_Released", _name, [&btn = _playerKeys]() { btn.up = false; });
	_events->AddListener("ArrowLeft_Pressed", _name, [&btn = _playerKeys]() { btn.left = true; });
	_events->AddListener("ArrowLeft_Released", _name, [&btn = _playerKeys]() { btn.left = false; });
	_events->AddListener("ArrowDown_Pressed", _name, [&btn = _playerKeys]() { btn.down = true; });
	_events->AddListener("ArrowDown_Released", _name, [&btn = _playerKeys]() { btn.down = false; });
	_events->AddListener("ArrowRight_Pressed", _name, [&btn = _playerKeys]() { btn.right = true; });
	_events->AddListener("ArrowRight_Released", _name, [&btn = _playerKeys]() { btn.right = false; });
	_events->AddListener("RCTRL_Pressed", _name, [&btn = _playerKeys]() { btn.shot = true; });
	_events->AddListener("RCTRL_Released", _name, [&btn = _playerKeys]() { btn.shot = false; });
	// for gamepad
	_events->AddListener("C2B_A_Pressed", _name, [&btn = _playerKeys]() { btn.shot = true; });
	_events->AddListener("C2B_A_Released", _name, [&btn = _playerKeys]() { btn.shot = false; });
	_events->AddListener("C2B_DPAD_UP_Pressed", _name, [&btn = _playerKeys]() { btn.up = true; });
	_events->AddListener("C2B_DPAD_UP_Released", _name, [&btn = _playerKeys]() { btn.up = false; });
	_events->AddListener("C2B_DPAD_DOWN_Pressed", _name, [&btn = _playerKeys]() { btn.down = true; });
	_events->AddListener("C2B_DPAD_DOWN_Released", _name, [&btn = _playerKeys]() { btn.down = false; });
	_events->AddListener("C2B_DPAD_RIGHT_Pressed", _name, [&btn = _playerKeys]() { btn.right = true; });
	_events->AddListener("C2B_DPAD_RIGHT_Released", _name, [&btn = _playerKeys]() { btn.right = false; });
	_events->AddListener("C2B_DPAD_LEFT_Pressed", _name, [&btn = _playerKeys]() { btn.left = true; });
	_events->AddListener("C2B_DPAD_LEFT_Released", _name, [&btn = _playerKeys]() { btn.left = false; });
}

InputProviderForPlayerTwo::~InputProviderForPlayerTwo()
{
	_events->RemoveListener("ArrowUp_Pressed", _name);
	_events->RemoveListener("ArrowUp_Released", _name);
	_events->RemoveListener("ArrowLeft_Pressed", _name);
	_events->RemoveListener("ArrowLeft_Released", _name);
	_events->RemoveListener("ArrowDown_Pressed", _name);
	_events->RemoveListener("ArrowDown_Released", _name);
	_events->RemoveListener("ArrowRight_Pressed", _name);
	_events->RemoveListener("ArrowRight_Released", _name);
	_events->RemoveListener("RCTRL_Pressed", _name);
	_events->RemoveListener("RCTRL_Released", _name);

	_events->RemoveListener("C2B_A_Pressed", _name);
	_events->RemoveListener("C2B_A_Released", _name);
	_events->RemoveListener("C2B_DPAD_UP_Pressed", _name);
	_events->RemoveListener("C2B_DPAD_UP_Released", _name);
	_events->RemoveListener("C2B_DPAD_DOWN_Pressed", _name);
	_events->RemoveListener("C2B_DPAD_DOWN_Released", _name);
	_events->RemoveListener("C2B_DPAD_RIGHT_Pressed", _name);
	_events->RemoveListener("C2B_DPAD_RIGHT_Released", _name);
	_events->RemoveListener("C2B_DPAD_LEFT_Pressed", _name);
	_events->RemoveListener("C2B_DPAD_LEFT_Released", _name);
}
