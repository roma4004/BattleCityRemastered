#include "components/ScoreBoard.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/GameStatistics.h"
#include "components/events/SpawnEvents.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/RenderUIEvents.h"
#include "enums/GameState.h"
#include "enums/RespawnGroup.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <iomanip>
#include <ranges>
#include <span>
#include <sstream>
#include <string_view>

namespace
{
using StatField = unsigned short StatisticsData::*;

//NOTE: the scoreboard is a table, so it is written as one - a label and up to three counters,
//columns in the order P1, P2, the third party. An unset column simply is not printed
inline constexpr std::size_t kMaxColumns{3};

//NOTE: in characters - the header row lines up with the counters by using the same width
inline constexpr int kLabelWidth{22};
inline constexpr int kColumnWidth{4};

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
}//namespace

ScoreBoard::ScoreBoard(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig)
	: _pos{.x = 25, .y = 25}
	, _events{events}
	, _statistics{std::make_unique<GameStatistics>(events)}
	, _gameConfig{gameConfig}
{
	Subscribe();

	_windowHeight = static_cast<int>(gameConfig.LogicalSize().y);
}

ScoreBoard::~ScoreBoard() = default;

void ScoreBoard::Subscribe()
{
	//NOTE: avoid showing score on game start
	_subs.push_back(_events->AddListener(this, &ScoreBoard::OnGameReset));

	_subs.push_back(_events->AddListener(this, &ScoreBoard::OnRespawnCountChangedTo));

	if (_isScoreBoardDisplayed)
	{
		_drawSub = _events->AddListener(this, &ScoreBoard::OnDrawUserInterface);
	}

	//NOTE: avoid showing score and menu at the same time
	_subs.push_back(_events->AddListener(this, &ScoreBoard::OnMenuShowed));
	_subs.push_back(_events->AddListener(this, &ScoreBoard::OnPauseStatus));
	_subs.push_back(_events->AddListener(this, &ScoreBoard::OnGameFinished));
}

void ScoreBoard::OnGameReset(const GameResetEvent&) { DisplayScore(false); }

void ScoreBoard::OnRespawnCountChangedTo(const RespawnCountChangedToEvent& event)
{
	switch (event.group)
	{
		case RespawnGroup::ENEMY_ALL:
			_enemyRespawnCount = event.respawnCount;
			return;
		case RespawnGroup::PLAYER1:
			_playerOneRepawnCount = event.respawnCount;
			return;
		case RespawnGroup::PLAYER2:
			_playerTwoRespawnCount = event.respawnCount;
			return;
	}
}

void ScoreBoard::OnDrawUserInterface(const DrawUserInterfaceEvent&) { Draw(); }

void ScoreBoard::OnMenuShowed(const MenuShowedEvent& event)
{
	if (event.isShown)
	{
		DisplayScore(false);
	}
}

void ScoreBoard::OnPauseStatus(const PauseStatusEvent& /*event*/)
{
	/*DisplayScore(isPause);*/
}

void ScoreBoard::OnGameFinished(const GameFinishedEvent&) { DisplayScore(true); }

void ScoreBoard::Draw() const
{
	_events->EmitEvent(RenderMenuBackgroundEvent{.pos = _pos});
	RenderStatistics();
}

void ScoreBoard::RenderStatistics() const
{
	const Point pos{.x = _pos.x + 180, .y = _pos.y + 120};
	constexpr unsigned int color{0xff00ffffu};

	_events->EmitEvent(
			RenderTextEvent{.pos = Point{.x = pos.x - 60, .y = pos.y + 80},
							.color = color,
							.text = "PRESS M TO SHOW MENU"});
	_events->EmitEvent(
			RenderTextEvent{.pos = Point{.x = pos.x - 20, .y = pos.y + 120},
							.color = color,
							.text = "GAME STATISTICS:"});

	std::ostringstream header;
	header << std::left;
	for (const std::string_view column: {"P1", "P2", "ENEMY"})
	{
		header << std::setw(kColumnWidth) << column;
	}

	_events->EmitEvent(RenderTextEvent{.pos = {.x = pos.x + 180, .y = pos.y + 140},
									   .color = color,
									   .text = header.str()});

	constexpr int rowStep{20};
	int y{pos.y + 160};

	//NOTE: the respawn counts are the scoreboard's own, not the statistics block's
	RenderRow({.x = pos.x - 130, .y = y}, color, "RESPAWN REMAIN",
			  std::array{_playerOneRepawnCount, _playerTwoRespawnCount, _enemyRespawnCount});

	const StatisticsData& data = _statistics->GetData();
	for (const auto& [label, columns]: kStatRows)
	{
		y += rowStep;

		//NOTE: a row fills its columns from the left, so the unset ones are the tail
		const auto filled = static_cast<std::size_t>(std::ranges::count_if(columns, [](const StatField field)
		{
			return field != nullptr;
		}));

		std::array<unsigned short, kMaxColumns> values{};
		std::ranges::transform(columns | std::views::take(filled), values.begin(),
							   [&data](const StatField field) { return data.*field; });

		RenderRow({.x = pos.x - 130, .y = y}, color, label, std::span{values}.first(filled));
	}
}

void ScoreBoard::RenderRow(const Point pos, const unsigned int color, const std::string_view text,
						   const std::span<const unsigned short> values) const
{
	std::ostringstream textStream;
	textStream << std::left << std::setw(kLabelWidth) << text;
	for (const unsigned short value: values)
	{
		textStream << std::setw(kColumnWidth) << value;
	}

	_events->EmitEvent(RenderTextEvent{.pos = pos, .color = color, .text = textStream.str()});
}

void ScoreBoard::DisplayScore(const bool isDisplayed)
{
	if (isDisplayed && _gameConfig.gameState == GameState::Demo)
	{
		return;
	}

	if (isDisplayed)
	{
		_events->EmitEvent(ShowMenuEvent{.show = false});
	}

	_isScoreBoardDisplayed = isDisplayed;

	if (_isScoreBoardDisplayed)
	{
		_drawSub = _events->AddListener(this, &ScoreBoard::OnDrawUserInterface);
	}
	else
	{
		_drawSub = EventSubscription{};
	}

	_events->EmitEvent(ScoreBoardShowedEvent{.isDisplayed = isDisplayed});
}
