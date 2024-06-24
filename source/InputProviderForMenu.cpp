#include "../headers/InputProviderForMenu.h"

InputProviderForMenu::InputProviderForMenu(std::string name, std::shared_ptr<EventSystem> events)
	: _name{std::move(name)}, _events{std::move(events)}
{
	_events->AddListener("Menu_Released", _name, [this]() { ToggleMenu(); });
	_events->AddListener("Pause_Released", _name, [this]() { keys.pause = !keys.pause; });
}

InputProviderForMenu::~InputProviderForMenu()
{
	_events->RemoveListener("Menu_Released", _name);
	_events->RemoveListener("Pause_Released", _name);
}

void InputProviderForMenu::ToggleMenu()
{
	keys.menuShow = !keys.menuShow;
	if (keys.menuShow)
	{
		_events->AddListener("ArrowUp_Released", _name, [&btn = keys]() { btn.up = true; });
		_events->AddListener("ArrowUp_Pressed", _name, [&btn = keys]() { btn.up = false; });
		_events->AddListener("ArrowDown_Released", _name, [&btn = keys]() { btn.down = true; });
		_events->AddListener("ArrowDown_Pressed", _name, [&btn = keys]() { btn.down = false; });
		_events->AddListener("Enter_Released", _name, [&btn = keys]() { btn.reset = true; });
	}
	else
	{
		_events->RemoveListener("ArrowUp_Released", _name);
		_events->RemoveListener("ArrowUp_Pressed", _name);
		_events->RemoveListener("ArrowDown_Released", _name);
		_events->RemoveListener("ArrowDown_Pressed", _name);
		_events->RemoveListener("Enter_Released", _name);
	}
}
