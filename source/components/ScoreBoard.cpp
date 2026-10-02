#include "components/ScoreBoard.h"
#include "components/EventSystem.h"
#include "components/GameStatistics.h"
#include "components/StatisticsData.h"
#include "components/events/SpawnEvents.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/UiTable.h"
#include "enums/GameState.h"
#include "enums/RespawnGroup.h"
#include "enums/UiIcon.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
using StatField = unsigned short StatisticsData::*;

//NOTE: a label and up to three counters, columns in the order P1, P2, the third party - an unset one is not shown
inline constexpr std::size_t kMaxColumns{3};

inline constexpr unsigned int kTextColor{0xff00ffffu};

struct StatRow final
{
	std::string_view label{};
	std::array<StatField, kMaxColumns> columns{};
};

constexpr std::array kStatRows{
		StatRow{.label = "BULLET HIT BY BULLET",
				.columns = {&StatisticsData::bulletHitByPlayerOne,
							&StatisticsData::bulletHitByPlayerTwo,
							&StatisticsData::bulletHitByEnemy}},
		StatRow{.label = "PLAYER HIT BY ENEMY",
				.columns = {&StatisticsData::playerOneHitByEnemyTeam,
							&StatisticsData::playerTwoHitByEnemyTeam}},
		StatRow{.label = "ENEMY HIT BY",
				.columns = {&StatisticsData::enemyHitByPlayerOne,
							&StatisticsData::enemyHitByPlayerTwo,
							&StatisticsData::enemyHitByFriendlyFire}},
		StatRow{.label = "TANK KILLS",
				.columns = {&StatisticsData::enemyDiedByPlayerOne,
							&StatisticsData::enemyDiedByPlayerTwo,
							&StatisticsData::playerDiedByEnemyTeam}},
		StatRow{.label = "FRIENDLY HITS TAKEN",
				.columns = {&StatisticsData::playerOneHitFriendlyFire,
							&StatisticsData::playerTwoHitFriendlyFire,
							&StatisticsData::enemyHitByFriendlyFire}},
		StatRow{.label = "FRIENDLY KILLS TAKEN",
				.columns = {&StatisticsData::playerOneDiedByFriendlyFire,
							&StatisticsData::playerTwoDiedByFriendlyFire,
							&StatisticsData::enemyDiedByFriendlyFire}},
		StatRow{.label = "BRICK KILLS",
				.columns = {&StatisticsData::brickWallDiedByPlayerOne,
							&StatisticsData::brickWallDiedByPlayerTwo,
							&StatisticsData::brickWallDiedByEnemyTeam}},
		StatRow{.label = "STEEL KILLS",
				.columns = {&StatisticsData::steelWallDiedByPlayerOne,
							&StatisticsData::steelWallDiedByPlayerTwo,
							&StatisticsData::steelWallDiedByEnemyTeam}},
		StatRow{.label = "BONUS PICKUPS",
				.columns = {&StatisticsData::bonusPickupByPlayerOne,
							&StatisticsData::bonusPickupByPlayerTwo,
							&StatisticsData::bonusPickupByEnemyTeam}},
		StatRow{.label = "BONUS DESTROYED",
				.columns = {&StatisticsData::bonusDestroyedByPlayerOne,
							&StatisticsData::bonusDestroyedByPlayerTwo,
							&StatisticsData::bonusDestroyedByEnemyTeam}},
		StatRow{.label = "BONUS EXPIRED", .columns = {&StatisticsData::bonusExpired}},
};

UiTable Caption(std::string text) { return UiTable{.rows = {UiRow{.cells = {TextCell(std::move(text), kTextColor)}}}}; }

UiRow RowOf(const std::string_view label, std::ranges::input_range auto&& values)
{
	UiRow row{.cells = {TextCell(std::string{label}, kTextColor)}};
	std::ranges::transform(values, std::back_inserter(row.cells),
						   [](const unsigned short value) { return TextCell(std::to_string(value), kTextColor); });

	return row;
}
}//namespace

ScoreBoard::ScoreBoard(const std::shared_ptr<EventSystem>& events, const GameStatistics& statistics)
	: _events{events}
	, _statistics{statistics}
{
	Subscribe();
}

void ScoreBoard::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &ScoreBoard::OnRespawnCountChangedTo));
	_subs.push_back(_events->AddListener(this, &ScoreBoard::OnMenuShown));
	_subs.push_back(_events->AddListener(this, &ScoreBoard::OnGameStateChangedTo));
}

void ScoreBoard::OnRespawnCountChangedTo(const RespawnCountChangedToEvent& event)
{
	switch (event.group)
	{
		case RespawnGroup::ENEMY_ALL:
			_enemyRespawnCount = event.respawnCount;
			return;
		case RespawnGroup::PLAYER1:
			_playerOneRespawnCount = event.respawnCount;
			return;
		case RespawnGroup::PLAYER2:
			_playerTwoRespawnCount = event.respawnCount;
			return;
	}
}

void ScoreBoard::OnDrawUserInterface(const DrawUserInterfaceEvent&) const { Draw(); }

void ScoreBoard::OnPreDrawUserInterface(const PreDrawUserInterfaceEvent&) const
{
	_events->EmitEvent(RenderPlateEvent{.plate = Plate()});
}

UiIcon ScoreBoard::Plate() const { return _isWon ? UiIcon::PlateGameWon : UiIcon::PlateGameOver; }

void ScoreBoard::OnMenuShown(const MenuShownEvent& event)
{
	if (event.isShown)
	{
		DisplayScore(false);
	}
}

//NOTE: driven by the phase, which a client gets over the wire - it never runs the win check itself
void ScoreBoard::OnGameStateChangedTo(const GameStateChangedToEvent& event)
{
	_isFinished = event.state == GameState::Won || event.state == GameState::Over;
	if (_isFinished)
	{
		_isWon = event.state == GameState::Won;
		DisplayScore(!_isDemo);

		return;
	}

	_isWon = false;

	_isDemo = event.state == GameState::Demo;

	DisplayScore(false);
}

void ScoreBoard::Draw() const
{
	_events->EmitEvent(RenderMenuBackgroundEvent{});
	RenderStatistics();
}

void ScoreBoard::RenderStatistics() const
{
	UiTable statistics{.rows = {UiRow{.cells = {TextCell("", kTextColor), TextCell("P1", kTextColor),
												TextCell("P2", kTextColor), TextCell("ENEMY", kTextColor)}}}};

	//NOTE: the respawn counts are the scoreboard's own, not the statistics block's
	statistics.rows.push_back(
			RowOf("RESPAWN REMAIN", std::array{_playerOneRespawnCount, _playerTwoRespawnCount, _enemyRespawnCount}));

	const StatisticsData& data{_statistics.GetData()};
	for (const auto& [label, columns]: kStatRows)
	{
		statistics.rows.push_back(
				RowOf(label, columns | std::views::take_while([](const StatField field) { return field != nullptr; })
									 | std::views::transform([&data](const StatField field) { return data.*field; })));
	}

	std::vector<UiTable> tables{};
	tables.reserve(4u);
	tables.push_back(UiTable{.rows = {UiRow{.cells = {UiCell{.icon = Plate()}}}}});
	tables.push_back(Caption(_isWon ? "PRESS ENTER FOR NEXT LEVEL, M FOR MENU" : "PRESS M TO SHOW MENU"));
	tables.push_back(Caption("GAME STATISTICS:"));
	tables.push_back(std::move(statistics));

	_events->EmitEvent(RenderPanelTablesEvent{.tables = std::move(tables)});
}

void ScoreBoard::DisplayScore(const bool isShown)
{
	if (isShown)
	{
		_events->EmitEvent(ShowMenuEvent{.isShown = false});
	}

	_isScoreBoardShown = isShown;

	if (_isScoreBoardShown)
	{
		_drawSub = _events->AddListener(this, &ScoreBoard::OnDrawUserInterface);
	}
	else
	{
		_drawSub = EventSubscription{};
	}

	_enterSub = _isScoreBoardShown && _isWon ? _events->AddListener(this, &ScoreBoard::OnEnter)
												 : EventSubscription{};
	_plateSub = _isFinished && !_isScoreBoardShown
						? _events->AddListener(this, &ScoreBoard::OnPreDrawUserInterface)
						: EventSubscription{};

	_events->EmitEvent(ScoreBoardShownEvent{.isShown = isShown});
}

//NOTE: on the release, and the offer is taken down with it - a held key would ask for a level per frame,
//and the press that started the match would count as an answer to the board it put up
void ScoreBoard::OnEnter(const EnterEvent& event)
{
	if (event.isPressed)
	{
		return;
	}

	_enterSub = EventSubscription{};
	_events->EmitEvent(NextLevelRequestedEvent{});
}
