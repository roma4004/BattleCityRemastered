#include "components/input/InputProviderForPlayerOne.h"
#include "components/EventSystem.h"

InputProviderForPlayerOne::InputProviderForPlayerOne(std::shared_ptr<EventSystem> events)
	: _events{std::move(events)}
{
	_events->AddListener("P1_Move_Up_Pressed", _name, [&btn = _playerKeys]() { btn.up = true; });
	_events->AddListener("P1_Move_Up_Released", _name, [&btn = _playerKeys]() { btn.up = false; });
	_events->AddListener("P1_Move_Left_Pressed", _name, [&btn = _playerKeys]() { btn.left = true; });
	_events->AddListener("P1_Move_Left_Released", _name, [&btn = _playerKeys]() { btn.left = false; });
	_events->AddListener("P1_Move_Down_Pressed", _name, [&btn = _playerKeys]() { btn.down = true; });
	_events->AddListener("P1_Move_Down_Released", _name, [&btn = _playerKeys]() { btn.down = false; });
	_events->AddListener("P1_Move_Right_Pressed", _name, [&btn = _playerKeys]() { btn.right = true; });
	_events->AddListener("P1_Move_Right_Released", _name, [&btn = _playerKeys]() { btn.right = false; });
	_events->AddListener("P1_Fire_Pressed", _name, [&btn = _playerKeys]() { btn.shot = true; });
	_events->AddListener("P1_Fire_Released", _name, [&btn = _playerKeys]() { btn.shot = false; });
}

InputProviderForPlayerOne::~InputProviderForPlayerOne()
{
	_events->RemoveListener("P1_Move_Up_Pressed", _name);
	_events->RemoveListener("P1_Move_Up_Released", _name);
	_events->RemoveListener("P1_Move_Left_Pressed", _name);
	_events->RemoveListener("P1_Move_Left_Released", _name);
	_events->RemoveListener("P1_Move_Down_Pressed", _name);
	_events->RemoveListener("P1_Move_Down_Released", _name);
	_events->RemoveListener("P1_Move_Right_Pressed", _name);
	_events->RemoveListener("P1_Move_Right_Released", _name);
	_events->RemoveListener("P1_Fire_Pressed", _name);
	_events->RemoveListener("P1_Fire_Released", _name);
}
