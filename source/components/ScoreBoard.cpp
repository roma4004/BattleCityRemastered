#include "components/ScoreBoard.h"
#include "application/GameConfig.h"
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
#include "enums/PlayerSlot.h"
#include "enums/RespawnGroup.h"
#include "enums/UiIcon.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <iterator>
#include <memory>
#include <ranges>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

namespace
{
using SeatField = unsigned short SeatStatistics::*;
using TeamField = unsigned short EnemyTeamStatistics::*;

inline constexpr unsigned int kTextColor{0xff00ffffu};

//NOTE: a column per seat, then the enemy team's; a row without that field leaves the cell empty
struct StatRow final
{
	std::string_view label{};
	SeatField seat{};
	TeamField team{};
};

constexpr std::array kStatRows{
		StatRow{.label = "BULLET HIT BY BULLET",
				.seat = &SeatStatistics::bulletHits,
				.team = &EnemyTeamStatistics::bulletHits},
		StatRow{.label = "PLAYER HIT BY ENEMY", .seat = &SeatStatistics::hitByEnemyTeam},
		StatRow{.label = "ENEMY HIT BY",
				.seat = &SeatStatistics::enemyHits,
				.team = &EnemyTeamStatistics::friendlyHitsTaken},
		StatRow{.label = "TANK KILLS", .seat = &SeatStatistics::enemyKills, .team = &EnemyTeamStatistics::playerKills},
		StatRow{.label = "FRIENDLY HITS TAKEN",
				.seat = &SeatStatistics::friendlyHitsTaken,
				.team = &EnemyTeamStatistics::friendlyHitsTaken},
		StatRow{.label = "FRIENDLY KILLS TAKEN",
				.seat = &SeatStatistics::friendlyKillsTaken,
				.team = &EnemyTeamStatistics::friendlyKillsTaken},
		StatRow{.label = "BRICK KILLS",
				.seat = &SeatStatistics::brickWallKills,
				.team = &EnemyTeamStatistics::brickWallKills},
		StatRow{.label = "STEEL KILLS",
				.seat = &SeatStatistics::steelWallKills,
				.team = &EnemyTeamStatistics::steelWallKills},
		StatRow{.label = "BONUS PICKUPS",
				.seat = &SeatStatistics::bonusPickups,
				.team = &EnemyTeamStatistics::bonusPickups},
		StatRow{.label = "BONUS DESTROYED",
				.seat = &SeatStatistics::bonusesDestroyed,
				.team = &EnemyTeamStatistics::bonusesDestroyed},
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

ScoreBoard::ScoreBoard(const std::shared_ptr<EventSystem>& events, const GameStatistics& statistics,
					   const GameConfig& gameConfig)
	: _events{events}
	, _statistics{statistics}
	, _gameConfig{gameConfig}
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
	_respawnCounts[static_cast<std::size_t>(event.group)] = event.respawnCount;
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
	const auto seats{kSlots | std::views::take(_gameConfig.SeatCount())};
	UiTable statistics{.rows = {UiRow{.cells = {TextCell("", kTextColor)}}}};
	std::ranges::transform(seats, std::back_inserter(statistics.rows.front().cells), [](const PlayerSlot slot)
	{
		return TextCell("P" + std::to_string(SeatIndex(slot) + 1u), kTextColor);
	});
	statistics.rows.front().cells.push_back(TextCell("ENEMY", kTextColor));

	//NOTE: the respawn counts are the scoreboard's own, not the statistics block's
	std::vector<unsigned short> lives{};
	std::ranges::transform(seats, std::back_inserter(lives), [this](const PlayerSlot slot)
	{
		return _respawnCounts[static_cast<std::size_t>(GroupOf(slot))];
	});
	lives.push_back(_respawnCounts[static_cast<std::size_t>(RespawnGroup::ENEMY_ALL)]);
	statistics.rows.push_back(RowOf("RESPAWN REMAIN", lives));

	const StatisticsData& data{_statistics.GetData()};
	for (const auto& [label, seatField, teamField]: kStatRows)
	{
		std::vector<unsigned short> values{};
		std::ranges::transform(seats, std::back_inserter(values),
							   [&data, seatField](const PlayerSlot slot) { return data.Seat(slot).*seatField; });
		if (teamField != nullptr)
		{
			values.push_back(data.enemyTeam.*teamField);
		}

		statistics.rows.push_back(RowOf(label, values));
	}

	statistics.rows.push_back(RowOf("BONUS EXPIRED", std::array{data.bonusExpired}));

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
