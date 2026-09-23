#include "components/ScoreBoard.h"
#include "components/EventSystem.h"
#include "components/GameStatistics.h"
#include "components/StatisticsData.h"
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
#include <utility>
#include <vector>

namespace
{
using StatField = unsigned short StatisticsData::*;

//NOTE: the scoreboard is a table, so it is written as one - a label and up to three counters,
//columns in the order P1, P2, the third party. An unset column simply is not printed
inline constexpr std::size_t kMaxColumns{3};

//NOTE: in characters - the header row lines up with the counters by using the same width
inline constexpr int kLabelWidth{22};
inline constexpr int kColumnWidth{4};

inline constexpr int kRowStep{20};

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

void AddRow(std::vector<TextBlockLine>& lines, const Point pos, const unsigned int color,
			const std::string_view text, const std::span<const unsigned short> values)
{
	std::ostringstream textStream;
	textStream << std::left << std::setw(kLabelWidth) << text;
	for (const unsigned short value: values)
	{
		textStream << std::setw(kColumnWidth) << value;
	}

	lines.push_back(TextBlockLine{.pos = pos, .color = color, .text = textStream.str()});
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
	_subs.push_back(_events->AddListener(this, &ScoreBoard::OnMenuShowed));
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

void ScoreBoard::OnMenuShowed(const MenuShowedEvent& event)
{
	if (event.isShown)
	{
		DisplayScore(false);
	}
}

//NOTE: driven by the phase, which a client gets over the wire - it never runs the win check itself
void ScoreBoard::OnGameStateChangedTo(const GameStateChangedToEvent& event)
{
	if (event.state == GameState::Won || event.state == GameState::Over)
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
	_events->EmitEvent(RenderMenuBackgroundEvent{.pos = _pos});
	RenderStatistics();
}

//NOTE: one block, not a line at a time - the renderer sizes and centres the table as a whole, which is
//what keeps its columns columns
void ScoreBoard::RenderStatistics() const
{
	//NOTE: the board's own top left - the captions and the table hang off it, nothing off the menu's anchor
	const Point origin{.x = _pos.x + 50, .y = _pos.y + 200};
	constexpr unsigned int color{0xff00ffffu};

	std::vector<TextBlockLine> lines;
	lines.push_back(TextBlockLine{.pos = Point{.x = origin.x + 70, .y = origin.y},
								  .color = color,
								  .text = _isWon ? "PRESS ENTER FOR NEXT LEVEL, M FOR MENU" : "PRESS M TO SHOW MENU"});
	lines.push_back(TextBlockLine{.pos = Point{.x = origin.x + 110, .y = origin.y + 40},
								  .color = color,
								  .text = "GAME STATISTICS:"});

	std::ostringstream header;
	header << std::left;
	for (const std::string_view column: {"P1", "P2", "ENEMY"})
	{
		header << std::setw(kColumnWidth) << column;
	}

	//NOTE: over the counters, a label's width to the right of where the rows start
	lines.push_back(TextBlockLine{.pos = Point{.x = origin.x + 310, .y = origin.y + 60},
								  .color = color,
								  .text = header.str()});

	int y{origin.y + 80};

	//NOTE: the respawn counts are the scoreboard's own, not the statistics block's
	AddRow(lines, {.x = origin.x, .y = y}, color, "RESPAWN REMAIN",
		   std::array{_playerOneRespawnCount, _playerTwoRespawnCount, _enemyRespawnCount});

	const StatisticsData& data{_statistics.GetData()};
	for (const auto& [label, columns]: kStatRows)
	{
		y += kRowStep;

		//NOTE: a row fills its columns from the left, so the unset ones are the tail
		const auto filled{static_cast<std::size_t>(std::ranges::count_if(columns, [](const StatField field)
		{
			return field != nullptr;
		}))};

		std::array<unsigned short, kMaxColumns> values{};
		std::ranges::transform(columns | std::views::take(filled), values.begin(),
							   [&data](const StatField field) { return data.*field; });

		AddRow(lines, {.x = origin.x, .y = y}, color, label, std::span{values}.first(filled));
	}

	_events->EmitEvent(RenderMenuTextBlockEvent{.menuPos = _pos,
												.lineHeight = kRowStep,
												.align = TextBlockAlign::CenteredBlock,
												.lines = std::move(lines)});
}

void ScoreBoard::DisplayScore(const bool isDisplayed)
{
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

	_enterSub = _isScoreBoardDisplayed && _isWon ? _events->AddListener(this, &ScoreBoard::OnEnter)
												 : EventSubscription{};

	_events->EmitEvent(ScoreBoardShowedEvent{.isDisplayed = isDisplayed});
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
