#include "components/RightSideBar.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/UiTable.h"
#include "enums/RespawnGroup.h"
#include "enums/UiIcon.h"
#include "geometry/Point.h"
#include <ranges>
#include <string>
#include <vector>

namespace
{
constexpr int kReservePerRow{2};
//NOTE: opaque on purpose - the engine does not promote a transparent alpha
constexpr unsigned int kCounterColor{0xff000002u};
//NOTE: off the middle of the picture's left edge - right of the tank, low on the picture
constexpr Point kLivesNumberOffset{.x = 38, .y = 17};
constexpr Point kStageNumberOffset{.x = 38, .y = 30};

UiCell Counter(const UiIcon picture, const unsigned short value, const Point offset)
{
	return UiCell{.text = std::to_string(value), .background = picture, .color = kCounterColor, .offset = offset};
}
}//namespace

RightSideBar::RightSideBar(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig)
	: _gameConfig{gameConfig}
	, _events{events}
{
	Subscribe();
}

void RightSideBar::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnDrawUserInterface));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnRespawnCountChangedTo));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnMapLoaded));
}

void RightSideBar::OnDrawUserInterface(const DrawUserInterfaceEvent&) const { Draw(); }

void RightSideBar::Draw() const
{
	_events->EmitEvent(RenderSideBarEvent{.enemies = EnemiesTable(), .counters = CountersTable()});
}

UiTable RightSideBar::EnemiesTable() const
{
	const auto toRow = [](auto&& tanks) { return UiRow{.cells = std::ranges::to<std::vector>(tanks)}; };

	return UiTable{.rows = std::views::repeat(UiCell{.icon = UiIcon::SideBarEnemyTank}, _enemiesRespawnCount)
						   | std::views::chunk(kReservePerRow) | std::views::transform(toRow)
						   | std::ranges::to<std::vector>()};
}

//NOTE: no second player, no row - the flag moves up into the place it leaves
UiTable RightSideBar::CountersTable() const
{
	UiTable table{
			.rows = {UiRow{.cells = {Counter(UiIcon::SideBarPlayerOne, _playerOneRespawnCount, kLivesNumberOffset)}}}};
	if (_gameConfig.HasSecondPlayer())
	{
		table.rows.push_back(
				UiRow{.cells = {Counter(UiIcon::SideBarPlayerTwo, _playerTwoRespawnCount, kLivesNumberOffset)}});
	}

	table.rows.push_back(UiRow{.cells = {Counter(UiIcon::SideBarStageFlag, _stageNumber, kStageNumberOffset)}});

	return table;
}

void RightSideBar::OnRespawnCountChangedTo(const RespawnCountChangedToEvent& event)
{
	switch (event.group)
	{
		case RespawnGroup::ENEMY_ALL:
			_enemiesRespawnCount = event.respawnCount;
			return;
		case RespawnGroup::PLAYER1:
			_playerOneRespawnCount = event.respawnCount;
			return;
		case RespawnGroup::PLAYER2:
			_playerTwoRespawnCount = event.respawnCount;
			return;
	}
}

void RightSideBar::OnMapLoaded(const MapLoadedEvent& event) { _stageNumber = event.stage; }
