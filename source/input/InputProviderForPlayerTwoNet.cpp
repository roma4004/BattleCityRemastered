#include "../../headers/input/InputProviderForPlayerTwoNet.h"
#include "../../headers/components/EventSystem.h"

InputProviderForPlayerTwoNet::InputProviderForPlayerTwoNet(std::shared_ptr<EventSystem> events)
	: _events{std::move(events)}
{
	_events->AddListener("ServerReceive_ArrowUp_Pressed", _name, [&btn = playerKeys]() { btn.up = true; });
	_events->AddListener("ServerReceive_ArrowUp_Released", _name, [&btn = playerKeys]() { btn.up = false; });
	_events->AddListener("ServerReceive_ArrowLeft_Pressed", _name, [&btn = playerKeys]() { btn.left = true; });
	_events->AddListener("ServerReceive_ArrowLeft_Released", _name, [&btn = playerKeys]() { btn.left = false; });
	_events->AddListener("ServerReceive_ArrowDown_Pressed", _name, [&btn = playerKeys]() { btn.down = true; });
	_events->AddListener("ServerReceive_ArrowDown_Released", _name, [&btn = playerKeys]() { btn.down = false; });
	_events->AddListener("ServerReceive_ArrowRight_Pressed", _name, [&btn = playerKeys]()
	{
		btn.right = true;
	});
	_events->AddListener("ServerReceive_ArrowRight_Released", _name, [&btn = playerKeys]()
	{
		btn.right = false;
	});
	_events->AddListener("ServerReceive_RCTRL_Pressed", _name, [&btn = playerKeys]() { btn.shot = true; });
	_events->AddListener("ServerReceive_RCTRL_Released", _name, [&btn = playerKeys]() { btn.shot = false; });
}

InputProviderForPlayerTwoNet::~InputProviderForPlayerTwoNet()
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
