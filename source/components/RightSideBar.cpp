#include "components/RightSideBar.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/UiTable.h"
#include "entities/EffectTarget.h"
#include "enums/Author.h"
#include "enums/BonusType.h"
#include "enums/Faction.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include "enums/PlayerSlot.h"
#include "enums/RespawnGroup.h"
#include "utils/Timer.h"
#include "utils/TimeUtils.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <concepts>
#include <cstddef>
#include <optional>
#include <ranges>
#include <variant>
#include <vector>

RightSideBar::RightSideBar(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig)
	: _gameConfig{gameConfig}
	, _events{events}
{
	Subscribe();
}

void RightSideBar::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnGameModeApplied));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnGameReset));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnRespawnCountChangedTo));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnMapLoaded));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnSeatHoldersAnnounced));
	_subs.push_back(_events->AddListener(this, &RightSideBar::OnBonusEffectsChanged));
}

void RightSideBar::Draw() const
{
	const unsigned short enemyReserve{RespawnCountOf(RespawnGroup::ENEMY_ALL)};
	_events->EmitEvent(RenderSideBarEvent{.enemies = SideBarEnemies{.reserve = enemyReserve,
																	.badges = EnemyBadges(),
																	.teamFrozen = SpentOn(FreezesEnemyTeam)},
										  .players = SideBarPlayers{.seats = Seats(),
																	.stage = _stageNumber,
																	.fortified = SpentOn(FortifiesBase),
																	.hint = HintTable()}});
}

//NOTE: a new badge takes the first empty cell under the reserve; one the reserve grew over is placed anew
void RightSideBar::PlaceEnemyBadges()
{
	const int reserve{std::min(static_cast<int>(RespawnCountOf(RespawnGroup::ENEMY_ALL)), kEnemyFrameCells)};
	std::erase_if(_enemyBadges, [this, reserve](const EnemyBadge& badge)
	{
		return badge.cell < reserve || !EnemyBadgeSpent(badge.seat, badge.type);
	});

	const auto cells{std::views::iota(reserve, kEnemyFrameCells)};
	for (const auto [type, seat]: std::views::cartesian_product(std::array{BonusType::Helmet, BonusType::Timer},
																 kEnemyAuthors))
	{
		const bool isPlaced{std::ranges::any_of(_enemyBadges, [type, seat](const EnemyBadge& badge)
		{
			return badge.type == type && badge.seat == seat;
		})};
		if (isPlaced || !EnemyBadgeSpent(seat, type))
		{
			continue;
		}

		const auto empty{std::ranges::find_if(cells, [this](const int cell)
		{
			return !std::ranges::contains(_enemyBadges, cell, &EnemyBadge::cell);
		})};
		if (empty != cells.end())
		{
			_enemyBadges.push_back(EnemyBadge{.seat = seat, .type = type, .cell = *empty});
		}
	}
}

std::vector<SideBarEnemyBadge> RightSideBar::EnemyBadges() const
{
	return _enemyBadges | std::views::transform([this](const EnemyBadge& badge)
	{
		return SideBarEnemyBadge{.cell = badge.cell,
								 .type = badge.type,
								 .spent = EnemyBadgeSpent(badge.seat, badge.type)};
	}) | std::ranges::to<std::vector>();
}

//NOTE: a free-for-all timer freezes every enemy seat but the one that took it
std::optional<float> RightSideBar::EnemyBadgeSpent(const Author seat, const BonusType type) const
{
	if (type == BonusType::Helmet)
	{
		return HelmetSpent(seat);
	}

	return SpentOn([seat](const RunningEffect& effect)
	{
		return FreezesSeat(effect.type, effect.target, effect.spared, Faction::Solo, seat);
	});
}

std::vector<SideBarSeat> RightSideBar::Seats() const
{
	const auto seatOf = [this](const PlayerSlot slot)
	{
		const Author seat{AuthorOf(slot)};
		const Faction side{FactionOf(seat, _gameConfig.Rules())};

		return SideBarSeat{.lives = LivesOf(slot),
						   .frozen = SpentOn([seat, side](const RunningEffect& effect)
						   {
							   return FreezesSeat(effect.type, effect.target, effect.spared, side, seat);
						   }),
						   .helmet = HelmetSpent(seat)};
	};

	return kSlots | std::views::take(_gameConfig.SeatCount()) | std::views::transform(seatOf)
		   | std::ranges::to<std::vector>();
}

//NOTE: an empty seat shows 0 lives; its stock is kept for whoever takes the seat
unsigned short RightSideBar::LivesOf(const PlayerSlot slot) const
{
	if (_holders[SeatIndex(slot)] == SeatHolder::Empty)
	{
		return 0u;
	}

	return RespawnCountOf(GroupOf(slot));
}

unsigned short RightSideBar::RespawnCountOf(const RespawnGroup group) const
{
	return _respawnCounts[static_cast<std::size_t>(group)];
}

std::optional<float> RightSideBar::SpentOn(const std::predicate<const RunningEffect&> auto& isIt) const
{
	const auto it{std::ranges::find_if(_effects, [&isIt](const RunningEffect& effect)
	{
		return IsRunning(effect) && isIt(effect);
	})};

	return it == _effects.end() ? std::nullopt : std::optional{Spent(*it)};
}

std::optional<float> RightSideBar::HelmetSpent(const Author seat) const
{
	return SpentOn([seat](const RunningEffect& effect)
	{
		return effect.type == BonusType::Helmet && effect.target == EffectTarget{seat};
	});
}

float RightSideBar::Spent(const RunningEffect& effect)
{
	const std::chrono::duration<float> left{effect.timer.TimeLeft(TimeUtils::Now())};
	const std::chrono::duration<float> duration{effect.duration};

	return 1.f - left / duration;
}

bool RightSideBar::IsRunning(const RunningEffect& effect) { return !effect.timer.IsCooldownFinish(); }

bool RightSideBar::FreezesEnemyTeam(const RunningEffect& effect)
{
	return effect.type == BonusType::Timer && effect.target == EffectTarget{Faction::EnemyTeam};
}

bool RightSideBar::FortifiesBase(const RunningEffect& effect)
{
	return effect.type == BonusType::Shovel && effect.target == EffectTarget{Faction::PlayerTeam};
}

bool RightSideBar::IsWatching() const
{
	const std::optional<PlayerSlot>& own{_gameConfig.ownSlot};

	return _gameConfig.IsClient() && own && IsInMatch(_gameConfig.gameState)
		   && _holders[SeatIndex(*own)] == SeatHolder::Bot;
}

std::optional<UiTable> RightSideBar::HintTable() const
{
	if (!IsWatching())
	{
		return std::nullopt;
	}

	return UiTable{.rows = {UiRow{.cells = {TextCell("MOVE OR FIRE", kSideBarTextColor, UiAlign::Centered)}},
							UiRow{.cells = {TextCell("TO PLAY", kSideBarTextColor, UiAlign::Centered)}}}};
}

//NOTE: a network seat is empty until the host says who took it
void RightSideBar::OnGameModeApplied(const GameModeAppliedEvent& event)
{
	_holders.fill(IsNetworkGame(event.mode) ? SeatHolder::Empty : SeatHolder::Player);
}

void RightSideBar::OnGameReset(const GameResetEvent&)
{
	_effects.clear();
	_enemyBadges.clear();
}

void RightSideBar::OnRespawnCountChangedTo(const RespawnCountChangedToEvent& event)
{
	_respawnCounts[static_cast<std::size_t>(event.group)] = event.respawnCount;
	PlaceEnemyBadges();
}

void RightSideBar::OnMapLoaded(const MapLoadedEvent& event) { _stageNumber = event.stage; }

void RightSideBar::OnSeatHoldersAnnounced(const SeatHoldersAnnouncedEvent& event) { _holders = event.holders; }

void RightSideBar::OnBonusEffectsChanged(const BonusEffectsChangedEvent& event)
{
	const auto toRunning = [](const BonusEffectTimeLeft& entry)
	{
		return RunningEffect{.type = entry.type,
							 .target = entry.target,
							 .spared = entry.spared,
							 .timer = Timer{entry.left},
							 .duration = entry.duration};
	};

	_effects = event.effects | std::views::transform(toRunning) | std::ranges::to<std::vector>();
	PlaceEnemyBadges();
}
