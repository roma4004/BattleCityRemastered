#pragma once

#include "geometry/Point.h"
#include "components/EventSystem.h"

#include <memory>
#include <vector>

struct RespawnCountChangedToEvent;
struct DrawUserInterfaceEvent;
struct MenuShowedEvent;
struct GameStateChangedToEvent;
class EventSystem;
class GameStatistics;

class ScoreBoard final
{
	Point _pos{.x = 25, .y = 25};

	std::shared_ptr<EventSystem> _events{nullptr};
	const GameStatistics& _statistics;
	std::vector<EventSubscription> _subs{};
	// Toggled at runtime by DisplayScore(), where _subs is filled once at construction and stays
	EventSubscription _drawSub{};

	bool _isScoreBoardDisplayed{};
	bool _isDemo{};

	unsigned short _enemyRespawnCount{20u};
	unsigned short _playerOneRespawnCount{3u};
	unsigned short _playerTwoRespawnCount{3u};

	void Subscribe();

	void OnRespawnCountChangedTo(const RespawnCountChangedToEvent& event);
	void OnDrawUserInterface(const DrawUserInterfaceEvent&) const;
	void OnMenuShowed(const MenuShowedEvent& event);
	void OnGameStateChangedTo(const GameStateChangedToEvent& event);

	void RenderStatistics() const;

	void DisplayScore(bool isDisplayed);

	void Draw() const;

public:
	ScoreBoard(const std::shared_ptr<EventSystem>& events, const GameStatistics& statistics);
};
