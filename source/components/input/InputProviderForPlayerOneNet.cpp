#include "components/input/InputProviderForPlayerOneNet.h"
#include "components/EventSystem.h"

InputProviderForPlayerOneNet::InputProviderForPlayerOneNet(const std::shared_ptr<EventSystem>& events)
	: _events{events} {}

InputProviderForPlayerOneNet::~InputProviderForPlayerOneNet()
{
	Unsubscribe();
}

void InputProviderForPlayerOneNet::Subscribe()
{
    _events->AddListener("ServerReceive_P1_Move_Up_Pressed", _name, [&btn = _playerKeys]() { btn.up = true; });
    _events->AddListener("ServerReceive_P1_Move_Up_Released", _name, [&btn = _playerKeys]() { btn.up = false; });
    _events->AddListener("ServerReceive_P1_Move_Left_Pressed", _name, [&btn = _playerKeys]() { btn.left = true; });
    _events->AddListener("ServerReceive_P1_Move_Left_Released", _name, [&btn = _playerKeys]() { btn.left = false; });
    _events->AddListener("ServerReceive_P1_Move_Down_Pressed", _name, [&btn = _playerKeys]() { btn.down = true; });
    _events->AddListener("ServerReceive_P1_Move_Down_Released", _name, [&btn = _playerKeys]() { btn.down = false; });
    _events->AddListener("ServerReceive_P1_Move_Right_Pressed", _name, [&btn = _playerKeys]() { btn.right = true; });
    _events->AddListener("ServerReceive_P1_Move_Right_Released", _name, [&btn = _playerKeys]() { btn.right = false; });
    _events->AddListener("ServerReceive_P1_Fire_Pressed", _name, [&btn = _playerKeys]() { btn.shot = true; });
    _events->AddListener("ServerReceive_P1_Fire_Released", _name, [&btn = _playerKeys]() { btn.shot = false; });
}

void InputProviderForPlayerOneNet::Unsubscribe() const
{
    _events->RemoveListener("ServerReceive_P1_Move_Up_Pressed", _name);
    _events->RemoveListener("ServerReceive_P1_Move_Up_Released", _name);
    _events->RemoveListener("ServerReceive_P1_Move_Left_Pressed", _name);
    _events->RemoveListener("ServerReceive_P1_Move_Left_Released", _name);
    _events->RemoveListener("ServerReceive_P1_Move_Down_Pressed", _name);
    _events->RemoveListener("ServerReceive_P1_Move_Down_Released", _name);
    _events->RemoveListener("ServerReceive_P1_Move_Right_Pressed", _name);
    _events->RemoveListener("ServerReceive_P1_Move_Right_Released", _name);
    _events->RemoveListener("ServerReceive_P1_Fire_Pressed", _name);
    _events->RemoveListener("ServerReceive_P1_Fire_Released", _name);
}

void InputProviderForPlayerOneNet::Enable()
{
	Subscribe();
}

void InputProviderForPlayerOneNet::Disable() const
{
	Unsubscribe();
}
