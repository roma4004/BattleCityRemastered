#pragma once

#include "Point.h"
#include "components/EventSystem.h"
#include "components/input/InputProviderForMenu.h"
#include <vector>

class EventSystem;
class GameStatistics;
class InputProviderForMenu;

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

	std::string _name{};

	GameMode _selectedGameMode{};
	bool _isMenuDisplayed{false};

	void Subscribe();

	void DrawTextLine(Point& posText, std::string text) const;
	void DrawMenuText() const;
	void DrawMenuLine(Point& posText, bool isSelected, std::string text) const;
	void DrawControlHints() const;
	void DisplayMenu(bool isDisplayed);

public:
	Menu(UPoint windowSize, const std::shared_ptr<EventSystem>& events);

	~Menu() = default;

	void Draw();

	[[nodiscard]] MenuKeys GetKeysStats() const { return _input->GetKeysStats(); }
};
