#pragma once

#include "geometry/Point.h"
#include "components/EventSystem.h"

#include <memory>
#include <span>
#include <string_view>
#include <vector>

struct GameResetEvent;
struct RespawnCountChangedToEvent;
struct DrawUserInterfaceEvent;
struct MenuShowedEvent;
struct PauseStatusEvent;
struct GameFinishedEvent;
class GameConfig;
class EventSystem;
class GameStatistics;

class ScoreBoard final
{
	Point _pos{};
	int _windowHeight{};

	std::shared_ptr<EventSystem> _events{nullptr};
	const GameStatistics& _statistics;
	std::vector<EventSubscription> _subs{};
	// Toggled at runtime by DisplayScore(), where _subs is filled once at construction and stays
	EventSubscription _drawSub{};

	bool _isScoreBoardDisplayed{false};

	unsigned short _enemyRespawnCount{20u};
	unsigned short _playerOneRepawnCount{3u};
	unsigned short _playerTwoRespawnCount{3u};
	const GameConfig& _gameConfig;

	void Subscribe();

	void OnGameReset(const GameResetEvent&);
	void OnRespawnCountChangedTo(const RespawnCountChangedToEvent& event);
	void OnDrawUserInterface(const DrawUserInterfaceEvent&);
	void OnMenuShowed(const MenuShowedEvent& event);
	void OnPauseStatus(const PauseStatusEvent&);
	void OnGameFinished(const GameFinishedEvent&);

	void RenderStatistics() const;
	void RenderRow(Point pos, unsigned int color, std::string_view text,
				   std::span<const unsigned short> values) const;

	void DisplayScore(bool isDisplayed);

	void Draw() const;

public:
	ScoreBoard(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig,
			   const GameStatistics& statistics);
};
