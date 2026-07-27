#pragma once

#include "Point.h"
#include "enums/GameMode.h"

#include <memory>


class EventSystem;

class Options final
{
	Point _pos;
	int _windowHeight;
	int _padding{};
	int _yOffsetStart{};
	std::string _name{};
	bool _isOptionsDisplayed{false};
	GameMode _gameMode{};

	std::shared_ptr<EventSystem> _events{nullptr};

	void Subscribe();
	void Unsubscribe() const;

	void DisplayOptions(bool isDisplayed);
	void DrawTextLine(Point& posText, std::string text) const;

public:
	Options(UPoint windowSize, const std::shared_ptr<EventSystem>& events);

	~Options();

	void Draw();
};
