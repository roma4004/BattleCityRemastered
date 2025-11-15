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
	_events->AddListener("ServerReceive_ArrowUp_Pressed", _name, [&btn = _playerKeys]() { btn.up = true; });
	_events->AddListener("ServerReceive_ArrowUp_Released", _name, [&btn = _playerKeys]() { btn.up = false; });
	_events->AddListener("ServerReceive_ArrowLeft_Pressed", _name, [&btn = _playerKeys]() { btn.left = true; });
	_events->AddListener("ServerReceive_ArrowLeft_Released", _name, [&btn = _playerKeys]() { btn.left = false; });
	_events->AddListener("ServerReceive_ArrowDown_Pressed", _name, [&btn = _playerKeys]() { btn.down = true; });
	_events->AddListener("ServerReceive_ArrowDown_Released", _name, [&btn = _playerKeys]() { btn.down = false; });
	_events->AddListener("ServerReceive_ArrowRight_Pressed", _name, [&btn = _playerKeys]() { btn.right = true; });
	_events->AddListener("ServerReceive_ArrowRight_Released", _name, [&btn = _playerKeys]() { btn.right = false; });
	_events->AddListener("ServerReceive_RCTRL_Pressed", _name, [&btn = _playerKeys]() { btn.shot = true; });
	_events->AddListener("ServerReceive_RCTRL_Released", _name, [&btn = _playerKeys]() { btn.shot = false; });
}

void InputProviderForPlayerTwoNet::Unsubscribe() const
{
	_events->RemoveListener("ServerReceive_ArrowUp_Pressed", _name);
	_events->RemoveListener("ServerReceive_ArrowUp_Released", _name);
	_events->RemoveListener("ServerReceive_ArrowLeft_Pressed", _name);
	_events->RemoveListener("ServerReceive_ArrowLeft_Released", _name);
	_events->RemoveListener("ServerReceive_ArrowDown_Pressed", _name);
	_events->RemoveListener("ServerReceive_ArrowDown_Released", _name);
	_events->RemoveListener("ServerReceive_ArrowRight_Pressed", _name);
	_events->RemoveListener("ServerReceive_ArrowRight_Released", _name);
	_events->RemoveListener("ServerReceive_RCTRL_Pressed", _name);
	_events->RemoveListener("ServerReceive_RCTRL_Released", _name);
}

void InputProviderForPlayerTwoNet::Enable()
{
	Subscribe();
}

void InputProviderForPlayerTwoNet::Disable() const
{
	Unsubscribe();
}
