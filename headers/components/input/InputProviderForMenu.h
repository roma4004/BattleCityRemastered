#pragma once

#include "components/EventSystem.h"
#include <memory>
#include <string>
#include <vector>

enum class GameMode : char8_t;
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
	std::string _name{"InputProviderForMenu"};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	// Toggled at runtime by EnableMenuInput()/DisableMenuInput(), independent of _subs's fixed
	// subscribe-once-at-construction lifetime - clearing this vector auto-unsubscribes just this group.
	std::vector<EventSubscription> _menuNavSubs{};
	GameMode _gameMode{};
	MenuKeys _keys{};

public:
	explicit InputProviderForMenu(const std::shared_ptr<EventSystem>& events);

	~InputProviderForMenu() = default;

	void Subscribe();

	void EnableMenuInput();
	void DisableMenuInput();
	void ToggleMenuInputSubscription();
	void Reset();
	void MenuUpdate();
	void TogglePause();
	// void SwitchPause(bool switchTo);
	bool GetPause() const;
	void SetPause(bool value);

	void ToggleUp();
	void ToggleDown();

	[[nodiscard]] MenuKeys GetKeysStats() const noexcept { return _keys; }
};
