#include "components/RightSideBar.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/UiTable.h"
#include "enums/Author.h"
#include "enums/BonusType.h"
#include "enums/Faction.h"
#include "enums/GameState.h"
#include "enums/PlayerSlot.h"
#include "enums/RespawnGroup.h"
#include "enums/UiIcon.h"
#include "geometry/Point.h"
#include "utils/Timer.h"
#include "utils/TimeUtils.h"
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <functional>
#include <optional>
#include <ranges>
#include <string>
#include <utility>
#include <variant>
#include <vector>

namespace
{
constexpr int kReservePerRow{2};
constexpr std::size_t kReserveCells{20u};
//NOTE: opaque on purpose - the engine does not promote a transparent alpha
constexpr unsigned int kCounterColor{0xff000002u};
//NOTE: off the middle of the picture's left edge - right of the tank, low on the picture
constexpr Point kLivesNumberOffset{.x = 38, .y = 17};
constexpr Point kStageNumberOffset{.x = 38, .y = 30};

UiCell Counter(const UiIcon picture, const unsigned short value, const Point offset)
{
	return UiCell{.text = std::to_string(value), .background = picture, .color = kCounterColor, .offset = offset};
}

UiIcon IconOf(const BonusType type)
{
	switch (type)
	{
		case BonusType::Timer:
			return UiIcon::SideBarBonusTimer;
		case BonusType::Helmet:
			return UiIcon::SideBarBonusHelmet;
		case BonusType::Shovel:
			return UiIcon::SideBarBonusShovel;
		default:
			break;//NOTE: the others have no clock
	}

	return UiIcon::None;
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
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnGameReset));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnRespawnCountChangedTo));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnMapLoaded));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnSeatHoldersAnnounced));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnBonusEffectsChanged));
}

void RightSideBar::Draw() const
{
	_events->EmitEvent(
			RenderSideBarEvent{.enemies = EnemiesTable(), .counters = CountersTable(), .hint = HintTable()});
}

UiTable RightSideBar::EnemiesTable() const
{
	const auto toRow = [](auto&& cells) { return UiRow{.cells = std::ranges::to<std::vector>(cells)}; };
	const auto isShown = [](const RunningEffect& effect) { return IsOnEnemies(effect) && IsRunning(effect); };
	auto shown{_effects | std::views::filter(isShown)};

	const std::size_t reserve{EnemyReserve()};
	auto cells{std::views::repeat(UiCell{.icon = UiIcon::SideBarEnemyTank}, reserve) | std::ranges::to<std::vector>()};
	const auto pastCell = [](const RunningEffect& effect) { return effect.reserveCell + 1u; };
	cells.resize(std::ranges::fold_left(shown | std::views::transform(pastCell), reserve, std::ranges::max));
	std::ranges::for_each(shown, [&cells](const RunningEffect& effect)
	{
		cells[effect.reserveCell] = FadingCell(effect);
	});

	return UiTable{.rows = cells | std::views::chunk(kReservePerRow) | std::views::transform(toRow)
						   | std::ranges::to<std::vector>()};
}

//NOTE: a row per seat, text past two - the atlas pictures only two; the hint takes the flag's place, no room for both
UiTable RightSideBar::CountersTable() const
{
	const auto livesOf = [this](const PlayerSlot slot)
	{
		return _respawnCounts[static_cast<std::size_t>(GroupOf(slot))];
	};
	const std::size_t seats{_gameConfig.SeatCount()};

	const auto seatCell = [&livesOf, seats](const PlayerSlot slot)
	{
		if (seats > 2u)
		{
			const std::string label{std::to_string(SeatIndex(slot) + 1u) + "P " + std::to_string(livesOf(slot))};

			return TextCell(label, kCounterColor);
		}

		const UiIcon picture{slot == PlayerSlot::P1 ? UiIcon::SideBarPlayerOne : UiIcon::SideBarPlayerTwo};

		return Counter(picture, livesOf(slot), kLivesNumberOffset);
	};
	const auto besideLives = [seats](UiCell cell)
	{
		if (seats <= 2u)
		{
			cell.offset = Point{.y = kLivesNumberOffset.y};
		}

		return cell;
	};

	UiTable table{};
	for (const PlayerSlot slot: kSlots | std::views::take(seats))
	{
		const Author seat{AuthorOf(slot)};
		const auto isFrozen = [seat](const RunningEffect& effect)
		{
			const bool isOnSide{effect.target == EffectTarget{FactionOf(seat)}
								|| effect.target == EffectTarget{Faction::Solo}};

			return effect.type == BonusType::Timer && isOnSide && effect.spared != seat;
		};
		const auto isShielded = [seat](const RunningEffect& effect)
		{
			return effect.type == BonusType::Helmet && effect.target == EffectTarget{seat};
		};

		table.rows.push_back(UiRow{.cells = {besideLives(EffectCell(isFrozen)), seatCell(slot),
											 besideLives(EffectCell(isShielded))}});
	}

	if (!IsWatching())
	{
		const auto isFortified = [](const RunningEffect& effect)
		{
			return effect.type == BonusType::Shovel && effect.target == EffectTarget{Faction::PlayerTeam};
		};

		UiCell fortified{EffectCell(isFortified)};
		fortified.offset = Point{.y = kStageNumberOffset.y};

		table.rows.push_back(UiRow{.cells = {fortified,
											 Counter(UiIcon::SideBarStageFlag, _stageNumber, kStageNumberOffset)}});
	}

	return table;
}

UiCell RightSideBar::EffectCell(const std::function<bool(const RunningEffect&)>& isIt) const
{
	const auto it{std::ranges::find_if(_effects, [&isIt](const RunningEffect& effect)
	{
		return IsRunning(effect) && isIt(effect);
	})};

	return it == _effects.end() ? UiCell{} : FadingCell(*it);
}

UiCell RightSideBar::FadingCell(const RunningEffect& effect)
{
	const std::chrono::duration<float> left{effect.timer.TimeLeft(TimeUtils::Now())};
	const std::chrono::duration<float> duration{effect.timer.cooldown};

	return UiCell{.icon = IconOf(effect.type), .faded = 1.f - left / duration};
}

bool RightSideBar::IsRunning(const RunningEffect& effect) { return !effect.timer.IsCooldownFinish(); }

bool RightSideBar::IsOnEnemies(const RunningEffect& effect)
{
	const Author* const seat{std::get_if<Author>(&effect.target)};
	const bool isEnemySide{effect.target == EffectTarget{Faction::EnemyTeam}
						   || effect.target == EffectTarget{Faction::Solo}};

	return (effect.type == BonusType::Timer && isEnemySide && FactionOf(effect.spared) != Faction::EnemyTeam)
		   || (effect.type == BonusType::Helmet && seat != nullptr && FactionOf(*seat) == Faction::EnemyTeam);
}

std::size_t RightSideBar::PickCell(const BonusType type) const
{
	const std::size_t reserve{EnemyReserve()};
	const auto isFree = [this](const std::size_t cell)
	{
		return std::ranges::none_of(_effects, [cell](const RunningEffect& held)
		{
			return held.reserveCell == cell && IsOnEnemies(held) && IsRunning(held);
		});
	};

	if (type == BonusType::Timer)
	{
		const auto inFrame{std::views::iota(reserve, std::max(reserve, kReserveCells)) | std::views::reverse};
		if (const auto bottom{std::ranges::find_if(inFrame, isFree)};
			bottom != inFrame.end())
		{
			return *bottom;
		}
	}

	return *std::ranges::find_if(std::views::iota(reserve), isFree);
}

std::size_t RightSideBar::EnemyReserve() const
{
	return _respawnCounts[static_cast<std::size_t>(RespawnGroup::ENEMY_ALL)];
}

bool RightSideBar::IsWatching() const
{
	const std::optional<PlayerSlot>& own{_gameConfig.ownSlot};

	return _gameConfig.IsClient() && own && IsInMatch(_gameConfig.gameState)
		   && _holders[SeatIndex(*own)] == SeatHolder::Bot;
}

UiTable RightSideBar::HintTable() const
{
	if (!IsWatching())
	{
		return UiTable{};
	}

	return UiTable{.rows = {UiRow{.cells = {TextCell("MOVE OR FIRE", kCounterColor, UiAlign::Centered)}},
							UiRow{.cells = {TextCell("TO PLAY", kCounterColor, UiAlign::Centered)}}}};
}

void RightSideBar::OnGameReset(const GameResetEvent&) { _effects.clear(); }

void RightSideBar::OnRespawnCountChangedTo(const RespawnCountChangedToEvent& event)
{
	_respawnCounts[static_cast<std::size_t>(event.group)] = event.respawnCount;
	if (event.group != RespawnGroup::ENEMY_ALL)
	{
		return;
	}

	//NOTE: a grown reserve covers taken cells
	std::ranges::for_each(_effects, [this, reserve = EnemyReserve()](RunningEffect& effect)
	{
		if (effect.reserveCell < reserve && IsOnEnemies(effect) && IsRunning(effect))
		{
			effect.reserveCell = PickCell(effect.type);
		}
	});
}

void RightSideBar::OnMapLoaded(const MapLoadedEvent& event) { _stageNumber = event.stage; }

void RightSideBar::OnSeatHoldersAnnounced(const SeatHoldersAnnouncedEvent& event) { _holders = event.holders; }

//NOTE: shown ones keep their cells, newcomers take free ones after them
void RightSideBar::OnBonusEffectsChanged(const BonusEffectsChangedEvent& event)
{
	const std::vector<RunningEffect> shown{std::exchange(_effects, {})};
	std::vector<RunningEffect> newcomers{};

	for (const BonusEffectTimeLeft& entry: event.effects)
	{
		RunningEffect effect{.type = entry.type,
							 .target = entry.target,
							 .spared = entry.spared,
							 .timer = Timer{entry.duration}};
		effect.timer.activateTime -= entry.duration - entry.left;

		const auto before{std::ranges::find_if(shown, [&entry](const RunningEffect& old)
		{
			return old.type == entry.type && old.target == entry.target;
		})};
		if (before == shown.end() || !IsOnEnemies(*before) || !IsRunning(*before))
		{
			newcomers.push_back(effect);
			continue;
		}

		effect.reserveCell = before->reserveCell;
		_effects.push_back(effect);
	}

	std::ranges::for_each(newcomers, [this](RunningEffect& effect)
	{
		effect.reserveCell = PickCell(effect.type);
		_effects.push_back(effect);
	});
}
