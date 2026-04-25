#pragma once

#include "Point.h"
#include <memory>

class EventSystem;
class GameStatistics;

class ScoreBoard final
{
	Point _pos;
	int _windowHeight;
	int _padding;

	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<GameStatistics> _statistics{nullptr};

	std::string _name{};
	bool _isScoreBoardDisplayed{true};

	//TODO: extract to separate sidebar class
	int _enemyRespawnCount{20};
	int _playerOneRepawnCount{3};
	int _playerTwoRespawnCount{3};

	void Subscribe();
	void Unsubscribe() const;

	void RenderStatistics() const;
	void RenderTextWithAlignment(Point pos, unsigned int color, const std::string& text, int player1, int player2,
								 int enemy = -1) const;
	void RenderTextWithAlignment(Point pos, unsigned int color, const std::string& text, const std::string& text2,
								 const std::string& text3) const;

	void OnRespawnCountChanged(const std::string& objectName, int respawnCount);
	void DisplayScore(bool isDisplayed);

public:
	ScoreBoard(UPoint windowSize, const std::shared_ptr<EventSystem>& events);

	~ScoreBoard();

	void Draw();
};
