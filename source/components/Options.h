#pragma once

#include "components/EventSystem.h"
#include "geometry/Point.h"
#include "enums/GameMode.h"

#include <memory>
#include <string>
#include <vector>

class EventSystem;
struct MenuShownEvent;

class Options final
{
	Point _pos;
	int _windowHeight;
	int _yOffsetStart{};
	int _padding{};
	std::string _name{};
	bool _isMenuDisplayed{true};
	bool _isOptionsDisplayed{false};
	GameMode _selectedGameMode{};
	GameMode _gameMode{};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	void Subscribe();

	void OnMenuShown(const MenuShownEvent& event);

	void DisplayOptions(bool isDisplayed);
	void DrawTextLine(Point& posText, std::string text) const;

public:
	Options(UPoint windowSize, const std::shared_ptr<EventSystem>& events);

	~Options();

	void Draw();
};
