#pragma once

#include <memory>
#include <string>

enum GameMode : char8_t;
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
	GameMode _gameMode;
	MenuKeys _keys;

public:
	InputProviderForMenu(std::string name, std::shared_ptr<EventSystem> events);

	~InputProviderForMenu();

	void Subscribe();
	void SubscribeAsClient();

	void Unsubscribe() const;
	void UnsubscribeAsClient() const;
	void TogglePause();

	void ToggleMenuInputSubscription();

	void ToggleUp();
	void ToggleDown();

	[[nodiscard]] MenuKeys GetKeysStats() const { return _keys; }
};
