#include "components/RightSideBar.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/UiTable.h"
#include "enums/PlayerSlot.h"
#include "enums/RespawnGroup.h"
#include "enums/UiIcon.h"
#include "geometry/Point.h"
#include <cstddef>
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

	const unsigned short enemies{_respawnCounts[static_cast<std::size_t>(RespawnGroup::ENEMY_ALL)]};

	return UiTable{.rows = std::views::repeat(UiCell{.icon = UiIcon::SideBarEnemyTank}, enemies)
						   | std::views::chunk(kReservePerRow) | std::views::transform(toRow)
						   | std::ranges::to<std::vector>()};
}

//NOTE: a row per seat, the flag moves up; past two seats they are text - the atlas pictures only two
UiTable RightSideBar::CountersTable() const
{
	const auto livesOf = [this](const PlayerSlot slot)
	{
		return _respawnCounts[static_cast<std::size_t>(GroupOf(slot))];
	};
	const std::size_t seats{_gameConfig.SeatCount()};

	UiTable table{};
	for (const PlayerSlot slot: kSlots | std::views::take(seats))
	{
		if (seats > 2u)
		{
			const std::string label{std::to_string(SeatIndex(slot) + 1u) + "P " + std::to_string(livesOf(slot))};
			table.rows.push_back(UiRow{.cells = {TextCell(label, kCounterColor)}});
			continue;
		}

		const UiIcon picture{slot == PlayerSlot::P1 ? UiIcon::SideBarPlayerOne : UiIcon::SideBarPlayerTwo};
		table.rows.push_back(UiRow{.cells = {Counter(picture, livesOf(slot), kLivesNumberOffset)}});
	}

	table.rows.push_back(UiRow{.cells = {Counter(UiIcon::SideBarStageFlag, _stageNumber, kStageNumberOffset)}});

	return table;
}

void RightSideBar::OnRespawnCountChangedTo(const RespawnCountChangedToEvent& event)
{
	_respawnCounts[static_cast<std::size_t>(event.group)] = event.respawnCount;
}

void RightSideBar::OnMapLoaded(const MapLoadedEvent& event) { _stageNumber = event.stage; }
