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
	_events->AddListener("P2_Move_Up_Pressed", _name, [&btn = _playerKeys]() { btn.up = true; });
	_events->AddListener("P2_Move_Up_Released", _name, [&btn = _playerKeys]() { btn.up = false; });
	_events->AddListener("P2_Move_Left_Pressed", _name, [&btn = _playerKeys]() { btn.left = true; });
	_events->AddListener("P2_Move_Left_Released", _name, [&btn = _playerKeys]() { btn.left = false; });
	_events->AddListener("P2_Move_Down_Pressed", _name, [&btn = _playerKeys]() { btn.down = true; });
	_events->AddListener("P2_Move_Down_Released", _name, [&btn = _playerKeys]() { btn.down = false; });
	_events->AddListener("P2_Move_Right_Pressed", _name, [&btn = _playerKeys]() { btn.right = true; });
	_events->AddListener("P2_Move_Right_Released", _name, [&btn = _playerKeys]() { btn.right = false; });
	_events->AddListener("P2_Fire_Pressed", _name, [&btn = _playerKeys]() { btn.shot = true; });
	_events->AddListener("P2_Fire_Released", _name, [&btn = _playerKeys]() { btn.shot = false; });
}

void InputProviderForPlayerTwo::Unsubscribe() const
{
	_events->RemoveListener("P2_Move_Up_Pressed", _name);
	_events->RemoveListener("P2_Move_Up_Released", _name);
	_events->RemoveListener("P2_Move_Left_Pressed", _name);
	_events->RemoveListener("P2_Move_Left_Released", _name);
	_events->RemoveListener("P2_Move_Down_Pressed", _name);
	_events->RemoveListener("P2_Move_Down_Released", _name);
	_events->RemoveListener("P2_Move_Right_Pressed", _name);
	_events->RemoveListener("P2_Move_Right_Released", _name);
	_events->RemoveListener("P2_Fire_Pressed", _name);
	_events->RemoveListener("P2_Fire_Released", _name);
}

void InputProviderForPlayerTwo::Enable()
{
	Subscribe();
}

void InputProviderForPlayerTwo::Disable() const
{
	Unsubscribe();
}
