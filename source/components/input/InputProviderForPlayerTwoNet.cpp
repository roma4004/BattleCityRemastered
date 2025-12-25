#include "components/input/InputProviderForPlayerTwoNet.h"
#include "components/EventSystem.h"

InputProviderForPlayerTwoNet::InputProviderForPlayerTwoNet(const std::shared_ptr<EventSystem>& events)
	: _events{events} {}

InputProviderForPlayerTwoNet::~InputProviderForPlayerTwoNet()
{
	Unsubscribe();
}

void InputProviderForPlayerTwoNet::Subscribe()
{
	_events->AddListener("ServerReceive_P2_Move_Up_Pressed", _name, [&btn = _playerKeys]() { btn.up = true; });
	_events->AddListener("ServerReceive_P2_Move_Up_Released", _name, [&btn = _playerKeys]() { btn.up = false; });
	_events->AddListener("ServerReceive_P2_Move_Left_Pressed", _name, [&btn = _playerKeys]() { btn.left = true; });
	_events->AddListener("ServerReceive_P2_Move_Left_Released", _name, [&btn = _playerKeys]() { btn.left = false; });
	_events->AddListener("ServerReceive_P2_Move_Down_Pressed", _name, [&btn = _playerKeys]() { btn.down = true; });
	_events->AddListener("ServerReceive_P2_Move_Down_Released", _name, [&btn = _playerKeys]() { btn.down = false; });
	_events->AddListener("ServerReceive_P2_Move_Right_Pressed", _name, [&btn = _playerKeys]() { btn.right = true; });
	_events->AddListener("ServerReceive_P2_Move_Right_Released", _name, [&btn = _playerKeys]() { btn.right = false; });
	_events->AddListener("ServerReceive_P2_Fire_Pressed", _name, [&btn = _playerKeys]() { btn.shot = true; });
	_events->AddListener("ServerReceive_P2_Fire_Released", _name, [&btn = _playerKeys]() { btn.shot = false; });
}

void InputProviderForPlayerTwoNet::Unsubscribe() const
{
	_events->RemoveListener("ServerReceive_P2_Move_Up_Pressed", _name);
	_events->RemoveListener("ServerReceive_P2_Move_Up_Released", _name);
	_events->RemoveListener("ServerReceive_P2_Move_Left_Pressed", _name);
	_events->RemoveListener("ServerReceive_P2_Move_Left_Released", _name);
	_events->RemoveListener("ServerReceive_P2_Move_Down_Pressed", _name);
	_events->RemoveListener("ServerReceive_P2_Move_Down_Released", _name);
	_events->RemoveListener("ServerReceive_P2_Move_Right_Pressed", _name);
	_events->RemoveListener("ServerReceive_P2_Move_Right_Released", _name);
	_events->RemoveListener("ServerReceive_P2_Fire_Pressed", _name);
	_events->RemoveListener("ServerReceive_P2_Fire_Released", _name);
}

void InputProviderForPlayerTwoNet::Enable()
{
	Subscribe();
}

void InputProviderForPlayerTwoNet::Disable() const
{
	Unsubscribe();
}
