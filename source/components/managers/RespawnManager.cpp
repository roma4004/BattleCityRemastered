#include "components/managers/RespawnManager.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "enums/Author.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include "enums/RespawnGroup.h"
#include "enums/TankType.h"
#include "network/commands/WorldSnapshot.h"
#include "utils/UuidUtils.h"
#include "enums/Faction.h"
#include "enums/PlayerSlot.h"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <ranges>

namespace
{
//NOTE: a map that does not say how many enemies it has sends this many
constexpr unsigned short kUnlistedEnemies{20u};

constexpr unsigned short kStartingLives{3u};

bool IsEnemyGroup(const RespawnGroup group) noexcept { return group == RespawnGroup::ENEMY_ALL; }

[[nodiscard]] constexpr std::size_t GroupIndex(const PlayerSlot slot) noexcept
{
	return static_cast<std::size_t>(GroupOf(slot));
}
}//namespace

RespawnManager::RespawnManager(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig)
	: _events{events}
	, _gameMode{gameConfig.gameMode}
	, _isFreeForAll{gameConfig.IsFreeForAll()}
	, _enemySeats{gameConfig.EnemySeats()}
{
	std::ranges::fill(_holders | std::views::take(gameConfig.SeatCount()), SeatHolder::Player);

	for (const TankType enemy: {TankType::ENEMY1, TankType::ENEMY2, TankType::ENEMY3, TankType::ENEMY4})
	{
		_slots.push_back(
				SpawnSlot{.uuid = UuidUtils::GetRandomUuid(), .type = enemy, .group = RespawnGroup::ENEMY_ALL});
	}

	for (const PlayerSlot slot: kSlots)
	{
		_slots.push_back(
				SpawnSlot{.uuid = UuidUtils::GetRandomUuid(), .type = PlayerTankOf(slot), .group = GroupOf(slot)});
	}

	Subscribe();
}

void RespawnManager::Subscribe()
{
	//NOTE: a client counts nothing - the host sends the counts, the snapshot to a late joiner
	if (IsClient(_gameMode))
	{
		_subs.push_back(_events->AddListener(this, &RespawnManager::OnWorldSnapshotReceived));

		return;
	}

	_subs.push_back(_events->AddListener(this, &RespawnManager::OnGameReset));
	_subs.push_back(_events->AddListener(this, &RespawnManager::OnNextLevelRequested));
	_subs.push_back(_events->AddListener(this, &RespawnManager::OnTankSpawn));
	_subs.push_back(_events->AddListener(this, &RespawnManager::OnTankDied));
	_subs.push_back(_events->AddListener(this, &RespawnManager::OnBonusTankPickup));
	_subs.push_back(_events->AddListener(this, &RespawnManager::OnPlayersBaseFinished));
	_subs.push_back(_events->AddListener(this, &RespawnManager::OnRespawnTanks));
	_subs.push_back(_events->AddListener(this, &RespawnManager::OnEnemyLineupLoaded));
	_subs.push_back(_events->AddListener(this, &RespawnManager::OnGoldenGrenadePickup));

	if (IsHost(_gameMode))
	{
		_subs.push_back(_events->AddListener(this, &RespawnManager::OnWorldSnapshotRequested));
		_subs.push_back(_events->AddListener(this, &RespawnManager::OnSeatsFilled));
		_subs.push_back(_events->AddListener(this, &RespawnManager::OnSeatHolderChanged));
	}

	ResetSpawn();
}

//NOTE: a level change keeps the lives the players have left - the enemy count is the new map's own
void RespawnManager::OnGameReset(const GameResetEvent& event) { ResetSpawn(event.keepsPlayerProgress); }

//NOTE: a life on top every level - a player who spent them all comes back
void RespawnManager::OnNextLevelRequested(const NextLevelRequestedEvent&)
{
	const auto isSeated = [this](const PlayerSlot slot) { return _holders[SeatIndex(slot)] != SeatHolder::Empty; };
	for (const PlayerSlot slot: kSlots | std::views::filter(isSeated))
	{
		ChangeRespawnCount(1, GroupOf(slot));
	}
}

//NOTE: per tank on the field, a protected one too - the blast spares it, the reserve does not
void RespawnManager::OnGoldenGrenadePickup(const BonusGoldenGrenadePickupEvent& event)
{
	const bool isEnemyTaker{!SlotOf(event.taker)};
	const auto isHit = [this, &event, isEnemyTaker](const SpawnSlot& slot)
	{
		return slot.isOnField && SeatOf(slot.type) != event.taker
			   && (_isFreeForAll || IsEnemyGroup(slot.group) != isEnemyTaker);
	};

	for (const SpawnSlot& slot: _slots | std::views::filter(isHit))
	{
		ChangeRespawnCount(-1, slot.group);
	}
}

//NOTE: the reset before it counted the default - a map that says how many enemies it has overrides it
void RespawnManager::OnEnemyLineupLoaded(const EnemyLineupLoadedEvent& event)
{
	if (!event.count || _enemySeats == 0u)
	{
		return;
	}

	SetRespawnCount(RespawnGroup::ENEMY_ALL, static_cast<unsigned short>(*event.count));
}

//NOTE: a free seat somebody sits in waits - no more of them than the group has lives
void RespawnManager::RecountSeats(const RespawnGroup group)
{
	const std::size_t lives{_respawnCount[static_cast<std::size_t>(group)]};
	//NOTE: a player group is one seat
	const std::size_t seats{IsEnemyGroup(group) ? _enemySeats : 1u};
	const auto isOfGroup = [group](const SpawnSlot& slot) { return slot.group == group; };
	const auto isFree = [this](const SpawnSlot& slot)
	{
		const std::optional<PlayerSlot> seat{SlotOf(slot.type)};

		return !slot.isOnField && (!seat || _holders[SeatIndex(*seat)] != SeatHolder::Empty);
	};
	auto slots{_slots | std::views::filter(isOfGroup)};
	std::ranges::for_each(slots, [](SpawnSlot& slot) { slot.isAvailable = false; });
	std::ranges::for_each(slots | std::views::take(seats) | std::views::filter(isFree) | std::views::take(lives),
						  [](SpawnSlot& slot) { slot.isAvailable = true; });
}

void RespawnManager::ResetRespawnStat(const bool keepsPlayerLives)
{
	//NOTE: with no seat to come in by, no enemy is owed - the players fight to the last one standing
	_respawnCount[static_cast<std::size_t>(RespawnGroup::ENEMY_ALL)] = _enemySeats > 0u ? kUnlistedEnemies : 0u;
	if (!keepsPlayerLives)
	{
		std::ranges::for_each(kSlots, [this](const PlayerSlot slot)
		{
			_respawnCount[GroupIndex(slot)] = kStartingLives;
		});
	}

	for (SpawnSlot& slot: _slots)
	{
		//NOTE: a tank that saw the level out lost no life - its seat gets it back to spend on the next map
		if (keepsPlayerLives && slot.isOnField && !IsEnemyGroup(slot.group))
		{
			++_respawnCount[static_cast<std::size_t>(slot.group)];
		}

		slot.isAvailable = false;
		slot.isOnField = false;
	}
}

void RespawnManager::ResetSpawn(const bool keepsPlayerLives)
{
	ResetRespawnStat(keepsPlayerLives);

	//NOTE: announced on every reset - a level change is the one reset that does not restore the counters on screen
	for (const std::size_t index: std::views::iota(std::size_t{}, kRespawnGroupCount))
	{
		const auto group{static_cast<RespawnGroup>(index)};
		RecountSeats(group);
		_events->EmitEvent(RespawnCountChangedToEvent{.group = group, .respawnCount = _respawnCount[index]});
	}
}

void RespawnManager::ChangeRespawnCount(const int delta, const RespawnGroup group)
{
	const auto id{static_cast<std::size_t>(group)};
	if (const int newCount{_respawnCount[id] + delta}; newCount >= 0)
	{
		SetRespawnCount(group, static_cast<unsigned short>(newCount));
	}
}

//NOTE: every change but a reset's comes here
void RespawnManager::SetRespawnCount(const RespawnGroup group, const unsigned short count)
{
	_respawnCount[static_cast<std::size_t>(group)] = count;
	RecountSeats(group);
	_events->EmitEvent(RespawnCountChangedToEvent{.group = group, .respawnCount = count});
}

//NOTE: the tanks on the field are the players' last
void RespawnManager::OnPlayersBaseFinished(const PlayersBaseFinishedEvent&)
{
	for (const PlayerSlot slot: kSlots)
	{
		SetRespawnCount(GroupOf(slot), 0u);
	}

	CheckMatchEnd();
}

void RespawnManager::OnBonusTankPickup(const BonusTankPickupEvent& event)
{
	if (FactionOf(event.author) == Faction::EnemyTeam)
	{
		ChangeRespawnCount(1, RespawnGroup::ENEMY_ALL);
	}
	else if (const std::optional<PlayerSlot> slot{SlotOf(event.author)})
	{
		ChangeRespawnCount(1, GroupOf(*slot));
	}
}

//NOTE: told before the reset that spawns by it
void RespawnManager::OnSeatsFilled(const SeatsFilledEvent& event) { _holders = event.holders; }

//NOTE: a seat with a life left gets a tank; one a bot drained gets the starting lives, a returning player's stays
void RespawnManager::OnSeatHolderChanged(const SeatHolderChangedEvent& event)
{
	_holders[SeatIndex(event.slot)] = event.to;
	SpawnSlot& seat{*std::ranges::find(_slots, PlayerTankOf(event.slot), &SpawnSlot::type)};
	if (event.to == SeatHolder::Empty)
	{
		Vacate(seat);

		return;
	}

	if (event.from == SeatHolder::Player || seat.isOnField || seat.isAvailable)
	{
		return;
	}

	const std::size_t group{GroupIndex(event.slot)};
	if (event.from == SeatHolder::Bot && _respawnCount[group] == 0u)
	{
		SetRespawnCount(seat.group, kStartingLives);
	}

	RecountSeats(seat.group);
}

//NOTE: removed, not killed - its life returns to the seat; the match ends here only if nobody is left to play
void RespawnManager::Vacate(SpawnSlot& seat)
{
	if (seat.isOnField)
	{
		ChangeRespawnCount(1, seat.group);
	}

	seat.isOnField = false;
	seat.isAvailable = false;
	CheckMatchEnd();
}

void RespawnManager::OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const
{
	std::ranges::copy(_respawnCount, event.snapshot.respawnCounts.begin());
}

void RespawnManager::OnWorldSnapshotReceived(const WorldSnapshotReceivedEvent& event)
{
	for (const std::size_t group: std::views::iota(std::size_t{}, kRespawnGroupCount))
	{
		SetRespawnCount(static_cast<RespawnGroup>(group), event.snapshot.respawnCounts[group]);
	}
}

void RespawnManager::OnTankSpawn(const TankSpawnEvent& event)
{
	const auto it{std::ranges::find(_slots, event.uuid, &SpawnSlot::uuid)};
	if (it == _slots.end())
	{
		return;
	}

	it->isAvailable = false;
	it->isOnField = true;
	ChangeRespawnCount(-1, it->group);
}

//NOTE: no players left loses; a free-for-all is won by the last one standing
void RespawnManager::CheckMatchEnd() const
{
	const std::size_t playersIn{PlayersStillIn()};
	if (playersIn == 0u)
	{
		_events->EmitEvent(GameFinishedEvent{.state = GameState::Over});

		return;
	}

	if (AreEnemiesGone() && (!_isFreeForAll || playersIn == 1u))
	{
		_events->EmitEvent(GameFinishedEvent{.state = GameState::Won});
	}
}

bool RespawnManager::AreEnemiesGone() const
{
	const auto isEnemyOnField = [](const SpawnSlot& slot) { return IsEnemyGroup(slot.group) && slot.isOnField; };
	return _respawnCount[static_cast<std::size_t>(RespawnGroup::ENEMY_ALL)] == 0u
		   && std::ranges::none_of(_slots, isEnemyOnField);
}

//NOTE: in while its tank stands or its seat waits
std::size_t RespawnManager::PlayersStillIn() const
{
	const auto isIn = [](const SpawnSlot& slot)
	{
		return !IsEnemyGroup(slot.group) && (slot.isOnField || slot.isAvailable);
	};

	return static_cast<std::size_t>(std::ranges::count_if(_slots, isIn));
}

void RespawnManager::OnTankDied(const TankDiedEvent& event)
{
	const auto it{std::ranges::find(_slots, event.uuid, &SpawnSlot::uuid)};
	if (it == _slots.end())
	{
		return;
	}

	it->isOnField = false;
	RecountSeats(it->group);
	CheckMatchEnd();
}

void RespawnManager::OnRespawnTanks(const RespawnTanksEvent&) const
{
	for (const auto& slot: _slots | std::ranges::views::filter([](const auto& s) { return s.isAvailable; }))
	{
		_events->EmitEvent(RespawnTankEvent{.type = slot.type, .uuid = slot.uuid});
	}
}
