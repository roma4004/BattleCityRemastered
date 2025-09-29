#include "components/input/InputProviderForPlayerOne.h"
#include "components/EventSystem.h"

InputProviderForPlayerOne::InputProviderForPlayerOne(std::shared_ptr<EventSystem> events)
	: _events{std::move(events)}
{	// for keybord
	_events->AddListener("W_Pressed", _name, [&btn = _playerKeys]() { btn.up = true; });
	_events->AddListener("W_Released", _name, [&btn = _playerKeys]() { btn.up = false; });
	_events->AddListener("A_Pressed", _name, [&btn = _playerKeys]() { btn.left = true; });
	_events->AddListener("A_Released", _name, [&btn = _playerKeys]() { btn.left = false; });
	_events->AddListener("S_Pressed", _name, [&btn = _playerKeys]() { btn.down = true; });
	_events->AddListener("S_Released", _name, [&btn = _playerKeys]() { btn.down = false; });
	_events->AddListener("D_Pressed", _name, [&btn = _playerKeys]() { btn.right = true; });
	_events->AddListener("D_Released", _name, [&btn = _playerKeys]() { btn.right = false; });
	_events->AddListener("Space_Pressed", _name, [&btn = _playerKeys]() { btn.shot = true; });
	_events->AddListener("Space_Released", _name, [&btn = _playerKeys]() { btn.shot = false; });
	_events->AddListener("Tab_Released", _name, [&btn = _playerKeys](){btn.controllerSwap = true;});
	// for gamepad
	_events->AddListener("CB_A_Pressed", _name, [&btn = _playerKeys]() { btn.shot = true; });
	_events->AddListener("CB_A_Released", _name, [&btn = _playerKeys]() { btn.shot = false; });
	_events->AddListener("CB_DPAD_UP_Pressed", _name, [&btn = _playerKeys]() { btn.up = true; });
	_events->AddListener("CB_DPAD_UP_Released", _name, [&btn = _playerKeys]() { btn.up = false; });
	_events->AddListener("CB_DPAD_DOWN_Pressed", _name, [&btn = _playerKeys]() { btn.down = true; });
	_events->AddListener("CB_DPAD_DOWN_Released", _name, [&btn = _playerKeys]() { btn.down = false; });
	_events->AddListener("CB_DPAD_RIGHT_Pressed", _name, [&btn = _playerKeys]() { btn.right = true; });
	_events->AddListener("CB_DPAD_RIGHT_Released", _name, [&btn = _playerKeys]() { btn.right = false; });
	_events->AddListener("CB_DPAD_LEFT_Pressed", _name, [&btn = _playerKeys]() { btn.left = true; });
	_events->AddListener("CB_DPAD_LEFT_Released", _name, [&btn = _playerKeys]() { btn.left = false; });		
}

InputProviderForPlayerOne::~InputProviderForPlayerOne()
{
	_events->RemoveListener("W_Pressed", _name);
	_events->RemoveListener("W_Released", _name);
	_events->RemoveListener("A_Pressed", _name);
	_events->RemoveListener("A_Released", _name);
	_events->RemoveListener("S_Pressed", _name);
	_events->RemoveListener("S_Released", _name);
	_events->RemoveListener("D_Pressed", _name);
	_events->RemoveListener("D_Released", _name);
	_events->RemoveListener("Space_Pressed", _name);
	_events->RemoveListener("Space_Released", _name);
	_events->RemoveListener("Tab_Released", _name);

	_events->RemoveListener("CB_A_Pressed", _name);
	_events->RemoveListener("CB_A_Released", _name);
	_events->RemoveListener("CB_DPAD_UP_Pressed", _name);
	_events->RemoveListener("CB_DPAD_UP_Released", _name);
	_events->RemoveListener("CB_DPAD_DOWN_Pressed", _name);
	_events->RemoveListener("CB_DPAD_DOWN_Released", _name);
	_events->RemoveListener("CB_DPAD_RIGHT_Pressed", _name);
	_events->RemoveListener("CB_DPAD_RIGHT_Released", _name);
	_events->RemoveListener("CB_DPAD_LEFT_Pressed", _name);
	_events->RemoveListener("CB_DPAD_LEFT_Released", _name);	
}
