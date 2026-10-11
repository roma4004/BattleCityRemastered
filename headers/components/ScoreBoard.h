#pragma once

#include "components/EventSystem.h"
#include "enums/RespawnGroup.h"
#include <array>
#include <memory>
#include <vector>

enum class UiIcon : char8_t;
struct RespawnCountChangedToEvent;
struct EnterEvent;
struct MenuShownEvent;
struct GameStateChangedToEvent;
class EventSystem;
class GameConfig;
class GameStatistics;

class ScoreBoard final
{
	std::shared_ptr<EventSystem> _events{nullptr};
	const GameStatistics& _statistics;
	const GameConfig& _gameConfig;
	std::vector<EventSubscription> _subs{};

	bool _isScoreBoardShown{};
	//NOTE: the match is over - its plate stands on the board, or in the field when there is no board
	bool _isFinished{};
	bool _isDemo{};
	//NOTE: only a won map offers the next one - a lost one is replayed from the menu
	bool _isWon{};
	// Held only while the offer is on screen, so Enter means the next level and nothing else
	EventSubscription _enterSub{};

	//NOTE: indexed by RespawnGroup
	std::array<unsigned short, kRespawnGroupCount> _respawnCounts{};

	void Subscribe();

	void OnRespawnCountChangedTo(const RespawnCountChangedToEvent& event);
	void OnMenuShown(const MenuShownEvent& event);
	void OnGameStateChangedTo(const GameStateChangedToEvent& event);
	void OnEnter(const EnterEvent& event);

	void RenderStatistics() const;
	[[nodiscard]] UiIcon Plate() const;

	void DisplayScore(bool isShown);

public:
	ScoreBoard(const std::shared_ptr<EventSystem>& events, const GameStatistics& statistics,
			   const GameConfig& gameConfig);

	[[nodiscard]] bool IsShown() const noexcept { return _isScoreBoardShown; }
	void Draw() const;
	//NOTE: the end of a match nobody shows the board for - the demo's, under the field's own plate
	void DrawPlate() const;
};
