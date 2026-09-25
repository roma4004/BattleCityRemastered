#pragma once

#include "Point.h"
#include "enums/GameMode.h"

#include <memory>


class InputProviderForMenu;
class EventSystem;

class Options final
{
	Point _pos;
	int _windowHeight;
	int _yOffsetStart{};
	int _padding{};
	std::string _name{};
	bool _isOptionsDisplayed{false};
	GameMode _gameMode{};
	GameMode _selectedGameMode{};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<InputProviderForMenu> _input{nullptr};

	void Subscribe();
	void Unsubscribe() const;

	void DisplayOptions(bool isDisplayed);
	void DrawTextLine(Point& posText, std::string text) const;

public:
	Options(UPoint windowSize, const std::shared_ptr<EventSystem>& events);

	~Options();

	void Draw();
};
