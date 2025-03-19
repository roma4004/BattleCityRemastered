#include "../../headers/input/InputProviderForPlayerTwo.h"
#include "../../headers/components/EventSystem.h"

InputProviderForPlayerTwo::InputProviderForPlayerTwo(std::string name, std::shared_ptr<EventSystem> events)
	: _name{std::move(name)}, _events{std::move(events)}
{
	_events->AddListener("ArrowUp_Pressed", _name, [&btn = playerKeys]() { btn.up = true; });
	_events->AddListener("ArrowUp_Released", _name, [&btn = playerKeys]() { btn.up = false; });
	_events->AddListener("ArrowLeft_Pressed", _name, [&btn = playerKeys]() { btn.left = true; });
	_events->AddListener("ArrowLeft_Released", _name, [&btn = playerKeys]() { btn.left = false; });
	_events->AddListener("ArrowDown_Pressed", _name, [&btn = playerKeys]() { btn.down = true; });
	_events->AddListener("ArrowDown_Released", _name, [&btn = playerKeys]() { btn.down = false; });
	_events->AddListener("ArrowRight_Pressed", _name, [&btn = playerKeys]() { btn.right = true; });
	_events->AddListener("ArrowRight_Released", _name, [&btn = playerKeys]() { btn.right = false; });
	_events->AddListener("RCTRL_Pressed", _name, [&btn = playerKeys]() { btn.shot = true; });
	_events->AddListener("RCTRL_Released", _name, [&btn = playerKeys]() { btn.shot = false; });
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
}
