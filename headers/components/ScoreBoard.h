#pragma once

#include "components/EventSystem.h"

#include <memory>
#include <vector>

enum class UiIcon : char8_t;
struct RespawnCountChangedToEvent;
struct DrawUserInterfaceEvent;
struct PreDrawUserInterfaceEvent;
struct EnterEvent;
struct MenuShownEvent;
struct GameStateChangedToEvent;
class EventSystem;
class GameStatistics;

class ScoreBoard final
{
	std::shared_ptr<EventSystem> _events{nullptr};
	const GameStatistics& _statistics;
	std::vector<EventSubscription> _subs{};
	// Toggled at runtime by DisplayScore(), where _subs is filled once at construction and stays
	EventSubscription _drawSub{};

	bool _isScoreBoardShown{};
	//NOTE: the match is over - its plate stands on the board, or in the field when there is no board
	bool _isFinished{};
	bool _isDemo{};
	//NOTE: only a won map offers the next one - a lost one is replayed from the menu
	bool _isWon{};
	// Held only while the offer is on screen, so Enter means the next level and nothing else
	EventSubscription _enterSub{};
	EventSubscription _plateSub{};

	unsigned short _enemyRespawnCount{20u};
	unsigned short _playerOneRespawnCount{3u};
	unsigned short _playerTwoRespawnCount{3u};

	void Subscribe();

	void OnRespawnCountChangedTo(const RespawnCountChangedToEvent& event);
	void OnDrawUserInterface(const DrawUserInterfaceEvent&) const;
	void OnPreDrawUserInterface(const PreDrawUserInterfaceEvent&) const;
	void OnMenuShown(const MenuShownEvent& event);
	void OnGameStateChangedTo(const GameStateChangedToEvent& event);
	void OnEnter(const EnterEvent& event);

	void RenderStatistics() const;
	[[nodiscard]] UiIcon Plate() const;

	void DisplayScore(bool isShown);

	void Draw() const;

public:
	ScoreBoard(const std::shared_ptr<EventSystem>& events, const GameStatistics& statistics);
};
