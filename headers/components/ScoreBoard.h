#pragma once

#include "Point.h"
#include "enums/GameMode.h"

#include <memory>

class EventSystem;
class GameStatistics;

class ScoreBoard final
{
	Point _pos{};
	int _windowHeight{};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<GameStatistics> _statistics{nullptr};

	std::string _name{};
	bool _isScoreBoardDisplayed{false};

	unsigned short _enemyRespawnCount{20u};
	unsigned short _playerOneRepawnCount{3u};
	unsigned short _playerTwoRespawnCount{3u};
	GameMode _gameMode{};

	void Subscribe();
	void Unsubscribe() const;

	void RenderStatistics() const;
	void RenderTextWithAlignment(Point pos, unsigned int color, const std::string& text, unsigned short player1,
								 unsigned short player2, unsigned short enemy) const;
	void RenderTextWithAlignment(Point pos, unsigned int color, const std::string& text, unsigned short player1,
								 unsigned short player2) const;
	void RenderTextWithAlignment(Point pos, unsigned int color, const std::string& text, const std::string& text2,
								 const std::string& text3) const;

	void OnRespawnCountChanged(const std::string& objectName, unsigned short respawnCount);
	void DisplayScore(bool isDisplayed);

public:
	ScoreBoard(UPoint windowSize, const std::shared_ptr<EventSystem>& events);

	~ScoreBoard();

	void Draw();
};
