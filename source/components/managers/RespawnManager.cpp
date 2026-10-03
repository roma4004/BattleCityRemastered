#include "components/managers/RespawnManager.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/WorldSnapshot.h"
#include "enums/Author.h"
#include "enums/GameMode.h"
#include "enums/RespawnGroup.h"
#include "enums/TankType.h"
#include "utils/UuidUtils.h"
#include "utils/Uuid.h"
#include "enums/Faction.h"
#include "enums/PlayerSlot.h"
#include <algorithm>
#include <cstddef>
#include <memory>
#include <optional>
#include <ranges>

namespace
{
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
	, _seatCount{gameConfig.SeatCount()}
	, _enemySeats{gameConfig.EnemySeats()}
{
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
	_subs.push_back(_events->AddListener(this, &RespawnManager::OnGameReset));
	_subs.push_back(_events->AddListener(this, &RespawnManager::OnTankSpawn));
	_subs.push_back(_events->AddListener(this, &RespawnManager::OnTankDied));
	_subs.push_back(_events->AddListener(this, &RespawnManager::OnBonusTankPickup));
	_subs.push_back(_events->AddListener(this, &RespawnManager::OnPlayersBaseFinished));
	_subs.push_back(_events->AddListener(this, &RespawnManager::OnRespawnTanks));

	if (IsClient(_gameMode))
	{
		_subs.push_back(_events->AddListener(this, &RespawnManager::OnBonusTankApplied));
		_subs.push_back(_events->AddListener(this, &RespawnManager::OnTankRespawned));
		_subs.push_back(_events->AddListener(this, &RespawnManager::OnWorldSnapshotReceived));
	}

	if (IsHost(_gameMode))
	{
		_subs.push_back(_events->AddListener(this, &RespawnManager::OnWorldSnapshotRequested));
	}

	ResetSpawn();
}

//NOTE: a level change keeps the lives the players have left - the enemy count is the new map's own
void RespawnManager::OnGameReset(const GameResetEvent& event) { ResetSpawn(event.keepsPlayerProgress); }

void RespawnManager::OnBonusTankPickup(const BonusTankPickupEvent& event) { OnBonusTank(event.author); }

void RespawnManager::OnPlayersBaseFinished(const PlayersBaseFinishedEvent&) { TriggerLastPlayersLife(); }

void RespawnManager::OnRespawnTanks(const RespawnTanksEvent&) { RespawnTanks(); }

void RespawnManager::OnBonusTankApplied(const BonusTankAppliedEvent& event) { OnBonusTank(event.author); }

void RespawnManager::SetEnemyNeedRespawn()
{
	const auto isEnemy = [](const SpawnSlot& slot) { return IsEnemyGroup(slot.group); };
	for (SpawnSlot& slot: _slots | std::views::filter(isEnemy) | std::views::take(_enemySeats))
	{
		slot.isAvailable = true;
	}
}

void RespawnManager::ResetRespawnStat(const bool keepsPlayerLives)
{
	_respawnCount[static_cast<std::size_t>(RespawnGroup::ENEMY_ALL)] = 20u;
	if (!keepsPlayerLives)
	{
		std::ranges::for_each(kSlots, [this](const PlayerSlot slot) { _respawnCount[GroupIndex(slot)] = 3u; });
	}

	for (SpawnSlot& slot: _slots)
	{
		slot.isAvailable = false;
		slot.isOnField = false;
	}

	_enemiesSpawnCount = 0u;
	_enemiesDeathCount = 0u;
	_playersSpawnCount = 0u;
	_playersDeathCount = 0u;
}

void RespawnManager::ResetSpawn(const bool keepsPlayerLives)
{
	ResetRespawnStat(keepsPlayerLives);
	SetPlayerNeedRespawn();
	SetEnemyNeedRespawn();

	//NOTE: said out loud on every reset - the counters on screen are told what the numbers are, and a
	//level change is the one reset that does not put them back where they started
	for (const std::size_t group: std::views::iota(std::size_t{}, kRespawnGroupCount))
	{
		_events->EmitEvent(RespawnCountChangedToEvent{.group = static_cast<RespawnGroup>(group),
													   .respawnCount = _respawnCount[group]});
	}
}

//NOTE: only the seats this match has
void RespawnManager::SetPlayerNeedRespawn()
{
	for (const PlayerSlot slot: kSlots | std::views::take(_seatCount))
	{
		std::ranges::find(_slots, PlayerTankOf(slot), &SpawnSlot::type)->isAvailable = true;
	}
}

void RespawnManager::ChangeRespawnCount(const int delta, RespawnGroup type)
{
	const auto id{static_cast<size_t>(type)};
	if (const int newCount{_respawnCount[id] + delta}; newCount >= 0)
	{
		_respawnCount[id] = static_cast<unsigned short>(newCount);
	}

	_events->EmitEvent(RespawnCountChangedToEvent{.group = type, .respawnCount = _respawnCount[id]});
}

void RespawnManager::TriggerLastPlayersLife()
{
	for (const PlayerSlot slot: kSlots)
	{
		_respawnCount[GroupIndex(slot)] = 0u;
		_events->EmitEvent(RespawnCountChangedToEvent{.group = GroupOf(slot), .respawnCount = 0u});
	}
}

void RespawnManager::OnBonusTank(const Author author)
{
	if (FactionOf(author) == Faction::EnemyTeam)
	{
		ChangeRespawnCount(1, RespawnGroup::ENEMY_ALL);
	}
	else if (const std::optional<PlayerSlot> slot{SlotOf(author)})
	{
		ChangeRespawnCount(1, GroupOf(*slot));
	}

	if (IsHost(_gameMode))
	{
		_events->EmitEvent(BonusTankAppliedEvent{.author = author});
	}
}

void RespawnManager::OnTankRespawned(const TankRespawnedEvent& event)
{
	//NOTE: a coop bot spends no life of the seat it sits in
	const TankType type{event.type};
	if (!SlotOf(type))
	{
		ChangeRespawnCount(-1, RespawnGroup::ENEMY_ALL);
	}
	else if (IsPlayerTank(type))
	{
		ChangeRespawnCount(-1, GroupOf(*SlotOf(type)));
	}
}

void RespawnManager::OnWorldSnapshotRequested(const WorldSnapshotRequestedEvent& event) const
{
	std::ranges::copy(_respawnCount, event.snapshot.respawnCounts.begin());
}

void RespawnManager::OnWorldSnapshotReceived(const WorldSnapshotReceivedEvent& event)
{
	for (const std::size_t group: std::views::iota(std::size_t{}, kRespawnGroupCount))
	{
		_respawnCount[group] = event.snapshot.respawnCounts[group];
		_events->EmitEvent(RespawnCountChangedToEvent{.group = static_cast<RespawnGroup>(group),
													   .respawnCount = _respawnCount[group]});
	}
}

void RespawnManager::OnTankSpawn(const TankSpawnEvent& event)
{
	const Uuid& uuid{event.uuid};
	if (const auto it{std::ranges::find(_slots, uuid, &SpawnSlot::uuid)};
		it != _slots.end())
	{
		it->isAvailable = false;
		it->isOnField = true;
		ChangeRespawnCount(-1, it->group);
		if (IsEnemyGroup(it->group))
		{
			++_enemiesSpawnCount;
		}
		else
		{
			++_playersSpawnCount;
		}
	}
}

//NOTE: a free-for-all is won by the last one standing
void RespawnManager::OnEnemyDied(const bool isAvailable)
{
	++_enemiesDeathCount;
	if (isAvailable == false && _enemiesSpawnCount == _enemiesDeathCount
		&& (!_isFreeForAll || PlayersStillIn() <= 1u))
	{
		_events->EmitEvent(GameFinishedEvent{.state = GameState::Won});
	}
}

void RespawnManager::OnPlayerDied(const bool isAvailable)
{
	++_playersDeathCount;
	if (isAvailable == false && _playersSpawnCount == _playersDeathCount)
	{
		_events->EmitEvent(GameFinishedEvent{.state = GameState::Over});

		return;
	}

	if (_isFreeForAll && AreEnemiesGone() && PlayersStillIn() == 1u)
	{
		_events->EmitEvent(GameFinishedEvent{.state = GameState::Won});
	}
}

bool RespawnManager::AreEnemiesGone() const
{
	return _respawnCount[static_cast<std::size_t>(RespawnGroup::ENEMY_ALL)] == 0u
		   && _enemiesSpawnCount == _enemiesDeathCount;
}

//NOTE: in while its tank stands or a life is left
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
	const Uuid& uuid{event.uuid};
	if (const auto it{std::ranges::find(_slots, uuid, &SpawnSlot::uuid)};
		it != _slots.end())
	{
		it->isOnField = false;
		it->isAvailable = _respawnCount[static_cast<size_t>(it->group)] > 0u;
		if (IsEnemyGroup(it->group))
		{
			OnEnemyDied(it->isAvailable);
		}
		else
		{
			OnPlayerDied(it->isAvailable);
		}
	}
}

void RespawnManager::RespawnTanks()
{
	for (const auto& slot: _slots | std::ranges::views::filter([](const auto& s) { return s.isAvailable; }))
	{
		_events->EmitEvent(RespawnTankEvent{.type = slot.type, .uuid = slot.uuid});
	}
}
