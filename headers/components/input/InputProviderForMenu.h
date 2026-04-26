#pragma once

#include <memory>
#include <string>

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
	GameMode _gameMode{};
	MenuKeys _keys{};
	bool _isScoreBoardDisplayed{false};

public:
	explicit InputProviderForMenu(const std::shared_ptr<EventSystem>& events);

	~InputProviderForMenu();

	void Subscribe();
	void Unsubscribe() const;

	void EnableMenuInput();
	void DisableMenuInput() const;
	void OnScoreBoardShowed(bool isDisplayed);
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
