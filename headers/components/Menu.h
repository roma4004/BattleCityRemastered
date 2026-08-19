#pragma once

#include "geometry/Point.h"
#include "components/EventSystem.h"
#include "components/input/InputProviderForMenu.h"
#include <string>
#include <vector>

class GameConfig;
class EventSystem;
class GameStatistics;
class InputProviderForMenu;
struct DrawUserInterfaceEvent;
struct SelectedGameModeChangedToEvent;
struct MenuShowedEvent;

class Menu final
{
	Point _pos{};
	int _windowHeight{};
	int _yOffsetStart{};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	// Toggled at runtime by DisplayMenu() (also the initial Subscribe() call, if the menu starts
	// shown), independent of _subs's fixed subscribe-once-at-construction lifetime.
	EventSubscription _drawSub{};
	std::unique_ptr<InputProviderForMenu> _input{nullptr};

	GameMode _selectedGameMode{};
	bool _isMenuDisplayed{false};

	void Subscribe();

	void OnDrawUserInterface(const DrawUserInterfaceEvent&);
	void OnSelectedGameModeChangedTo(const SelectedGameModeChangedToEvent& event);
	void OnMenuShowed(const MenuShowedEvent& event);

	void DrawTextLine(Point& posText, std::string text) const;
	void DrawMenuText() const;
	void DrawMenuLine(Point& posText, bool isSelected, std::string text) const;
	void DrawControlHints() const;
	void DisplayMenu(bool isDisplayed);

public:
	Menu(const std::shared_ptr<EventSystem>& events, GameConfig& gameConfig);

	~Menu() = default;

	void Draw();

	[[nodiscard]] MenuKeys GetKeysStats() const { return _input->GetKeysStats(); }
};
