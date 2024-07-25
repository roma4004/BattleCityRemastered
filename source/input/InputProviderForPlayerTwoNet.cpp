#include "../../headers/input/InputProviderForPlayerTwoNet.h"
#include "../../headers/EventSystem.h"

InputProviderForPlayerTwoNet::InputProviderForPlayerTwoNet(std::string name, std::shared_ptr<EventSystem> events)
	: _name{std::move(name)}, _events{std::move(events)}
{
	_events->AddListener("Net_ArrowUp_Pressed", _name, [&btn = playerKeys]() { btn.up = true; });
	_events->AddListener("Net_ArrowUp_Released", _name, [&btn = playerKeys]() { btn.up = false; });
	_events->AddListener("Net_ArrowLeft_Pressed", _name, [&btn = playerKeys]() { btn.left = true; });
	_events->AddListener("Net_ArrowLeft_Released", _name, [&btn = playerKeys]() { btn.left = false; });
	_events->AddListener("Net_ArrowDown_Pressed", _name, [&btn = playerKeys]() { btn.down = true; });
	_events->AddListener("Net_ArrowDown_Released", _name, [&btn = playerKeys]() { btn.down = false; });
	_events->AddListener("Net_ArrowRight_Pressed", _name, [&btn = playerKeys]()
	{
		btn.right = true;
	});
	_events->AddListener("Net_ArrowRight_Released", _name, [&btn = playerKeys]()
	{
		btn.right = false;
	});
	_events->AddListener("Net_RCTRL_Pressed", _name, [&btn = playerKeys]() { btn.shot = true; });
	_events->AddListener("Net_RCTRL_Released", _name, [&btn = playerKeys]() { btn.shot = false; });
}

InputProviderForPlayerTwoNet::~InputProviderForPlayerTwoNet()
{
	_events->RemoveListener("Net_ArrowUp_Pressed", _name);
	_events->RemoveListener("Net_ArrowUp_Released", _name);
	_events->RemoveListener("Net_ArrowLeft_Pressed", _name);
	_events->RemoveListener("Net_ArrowLeft_Released", _name);
	_events->RemoveListener("Net_ArrowDown_Pressed", _name);
	_events->RemoveListener("Net_ArrowDown_Released", _name);
	_events->RemoveListener("Net_ArrowRight_Pressed", _name);
	_events->RemoveListener("Net_ArrowRight_Released", _name);
	_events->RemoveListener("Net_RCTRL_Pressed", _name);
	_events->RemoveListener("Net_RCTRL_Released", _name);
}
