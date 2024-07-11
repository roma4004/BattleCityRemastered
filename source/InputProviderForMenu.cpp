#include "../headers/InputProviderForMenu.h"

InputProviderForMenu::InputProviderForMenu(std::string name, std::shared_ptr<EventSystem> events)
	: _name{std::move(name)}, _events{std::move(events)}
{
	this->ToggleMenu();
	_events->AddListener("Menu_Released", _name, [this]() { this->ToggleMenu(); });
	_events->AddListener("Pause_Released", _name, [this]()
	{
		keys.pause = !keys.pause;

		_events->EmitEvent<const bool>("Pause_Status", keys.pause);
	});
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
		_events->AddListener("ArrowDown_Released", _name, [&btn = keys]() { btn.down = true; });
		_events->AddListener("Enter_Pressed", _name, [&btn = keys]() { btn.reset = true; });
		_events->AddListener("Enter_Released", _name, [&btn = keys]() { btn.reset = false; });
	}
	else
	{
		_events->RemoveListener("ArrowUp_Released", _name);
		_events->RemoveListener("ArrowDown_Released", _name);
		_events->RemoveListener("Enter_Pressed", _name);
		_events->RemoveListener("Enter_Released", _name);
	}

	keys.reset = false;
}

void InputProviderForMenu::ToggleUp()
{
	keys.up = false;
}

void InputProviderForMenu::ToggleDown()
{
	keys.down = false;
}
