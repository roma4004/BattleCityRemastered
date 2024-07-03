#pragma once

#include "EventSystem.h"

#include <memory>
#include <string>

struct MenuKeys
{
	bool up{false};
	bool down{false};
	bool reset{false};
	bool menuShow{false};
	bool pause{false};
};

class InputProviderForMenu
{
public:
	InputProviderForMenu(std::string name, std::shared_ptr<EventSystem> events);

	~InputProviderForMenu();

	void ToggleMenu();

	MenuKeys keys;

private:
	std::string _name;
	std::shared_ptr<EventSystem> _events;
};
