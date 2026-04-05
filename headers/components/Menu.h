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

	void Subscribe();
	void Unsubscribe() const;

	void RenderStatistics(Point pos) const;
	void RenderTextWithAlignment(Point pos, unsigned int color, const std::string& text, int player1, int player2,
								 int enemy = -1) const;
	void RenderTextWithAlignment(Point pos, unsigned int color, const std::string& text, const std::string& text2,
								 const std::string& text3) const;
	void DrawTextLine(Point& posText, bool isSelected, std::string text) const;
	void DrawText() const;

	void OnRespawnCountChanged(const std::string& objectName, int respawnCount);

public:
	Menu(UPoint windowSize, const std::shared_ptr<EventSystem>& events);

	~Menu();

	void MenuUpdate() const;
	void DrawMenu();

	[[nodiscard]] MenuKeys GetKeysStats() const { return _input->GetKeysStats(); }
};
