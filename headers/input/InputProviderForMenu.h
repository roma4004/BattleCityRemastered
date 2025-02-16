#pragma once

#include <memory>
#include <string>

class EventSystem;

struct MenuKeys final
{
	bool up{false};
	bool down{false};
	bool reset{false};
	bool menuShow{false};
	bool pause{false};
};

class InputProviderForMenu final
{
	std::string _name;
	std::shared_ptr<EventSystem> _events{nullptr};

	MenuKeys keys;

public:
	InputProviderForMenu(std::string name, std::shared_ptr<EventSystem> events);

	~InputProviderForMenu();

	void ToggleMenu();

	void ToggleUp();
	void ToggleDown();

	[[nodiscard]] MenuKeys GetKeysStats() const { return keys; }
};
