#pragma once

#include "Point.h"
#include "components/input/InputProviderForMenu.h"

class EventSystem;
class GameStatistics;
class InputProviderForMenu;

class Menu final
{
	Point _pos;
	int _windowHeight;
	int _padding;
	unsigned int _yOffsetStart{};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<GameStatistics> _statistics{nullptr};//TODO: extract from menu when we have dedicated screen
	std::unique_ptr<InputProviderForMenu> _input{nullptr};

	std::string _name{};

	//TODO: extract to separate sidebar class
	int _enemyRespawnCount{20};
	int _playerOneRepawnCount{3};
	int _playerTwoRespawnCount{3};
	GameMode _selectedGameMode{};
	bool _isMenuDisplayed{true};

	void Subscribe();
	void Unsubscribe() const;

	void DrawTextLine(Point& posText, std::string text) const;
	void DrawMenuText() const;
	void DrawMenuLine(Point& posText, bool isSelected, std::string text) const;
	void DrawControlHints() const;
	void DisplayMenu(bool isDisplayed);

public:
	Menu(UPoint windowSize, const std::shared_ptr<EventSystem>& events);

	~Menu();

	void Draw();

	[[nodiscard]] MenuKeys GetKeysStats() const { return _input->GetKeysStats(); }
};
