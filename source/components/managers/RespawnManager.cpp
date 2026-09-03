#include "components/managers/RespawnManager.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "enums/GameMode.h"
#include "enums/RespawnGroup.h"
#include "enums/TankType.h"
#include "utils/UuidUtils.h"
#include "utils/Uuid.h"
#include "enums/Faction.h"
#include <memory>
#include <ranges>

RespawnManager::RespawnManager(const std::shared_ptr<EventSystem>& events, const GameMode gameMode)
	: _events{events}
	, _gameMode{gameMode}
{
	_slots = {
			{.uuid = UuidUtils::GetRandomUuid(), .type = TankType::ENEMY1, .group = RespawnGroup::ENEMY_ALL},
			{.uuid = UuidUtils::GetRandomUuid(), .type = TankType::ENEMY2, .group = RespawnGroup::ENEMY_ALL},
			{.uuid = UuidUtils::GetRandomUuid(), .type = TankType::ENEMY3, .group = RespawnGroup::ENEMY_ALL},
			{.uuid = UuidUtils::GetRandomUuid(), .type = TankType::ENEMY4, .group = RespawnGroup::ENEMY_ALL},
			{.uuid = UuidUtils::GetRandomUuid(), .type = TankType::PLAYER1, .group = RespawnGroup::PLAYER1},
			{.uuid = UuidUtils::GetRandomUuid(), .type = TankType::PLAYER2, .group = RespawnGroup::PLAYER2},
	};

	Subscribe();
}

void RespawnManager::Subscribe()
{
	//TODO: reuse existing tanks when game mode changed
	//TODO: need work phase, clearState (all spawns disabled), battleState (spawn as normal)
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
	}

	ResetSpawn();
}

void RespawnManager::OnGameReset(const GameResetEvent&) { ResetSpawn(); }

void RespawnManager::OnBonusTankPickup(const BonusTankPickupEvent& event) { OnBonusTank(event.author); }

void RespawnManager::OnPlayersBaseFinished(const PlayersBaseFinishedEvent&) { TriggerLastPlayersLife(); }

void RespawnManager::OnRespawnTanks(const RespawnTanksEvent&) { RespawnTanks(); }

void RespawnManager::OnBonusTankApplied(const BonusTankAppliedEvent& event) { OnBonusTank(event.author); }

void RespawnManager::SetEnemyNeedRespawn()
{
	for (size_t i = 0u; i < 4u; ++i)
	{
		_slots[i].isAvailable = true;
	}
}

void RespawnManager::ResetRespawnStat()
{
	_respawnCount[static_cast<int>(RespawnGroup::ENEMY_ALL)] = 20u;
	_respawnCount[static_cast<int>(RespawnGroup::PLAYER1)] = 3u;
	_respawnCount[static_cast<int>(RespawnGroup::PLAYER2)] = 3u;

	for (auto& [uuid, tankType, respawnGroup, isAvailable]: _slots)
	{
		isAvailable = false;
	}

	_enemiesSpawnCount = 0u;
	_enemiesDeathCount = 0u;
	_playersSpawnCount = 0u;
	_playersDeathCount = 0u;
}

//NOTE: a reset means a fresh match, so every seat is owed a tank again - ResetRespawnStat clears the
//availability flags and nothing else ever sets them back
void RespawnManager::ResetSpawn()
{
	ResetRespawnStat();
	SetPlayerNeedRespawn();
	SetEnemyNeedRespawn();
}

void RespawnManager::SetPlayerNeedRespawn()
{
	constexpr auto player1Id = static_cast<size_t>(TankType::PLAYER1);
	_slots[player1Id].isAvailable = true;

	if (HasSecondPlayer(_gameMode))
	{
		constexpr auto player2Id = static_cast<size_t>(TankType::PLAYER2);
		_slots[player2Id].isAvailable = true;
	}
}

void RespawnManager::ChangeRespawnCount(const int delta, RespawnGroup type)
{
	const auto id = static_cast<size_t>(type);
	if (const int newCount = _respawnCount[id] + delta; newCount >= 0)
	{
		_respawnCount[id] = static_cast<unsigned short>(newCount);
	}

	_events->EmitEvent(RespawnCountChangedToEvent{.group = type, .respawnCount = _respawnCount[id]});
}

void RespawnManager::TriggerLastPlayersLife()
{
	_respawnCount[1] = 0u;
	_events->EmitEvent(RespawnCountChangedToEvent{.group = RespawnGroup::PLAYER1, .respawnCount = _respawnCount[1]});
	_respawnCount[2] = 0u;
	_events->EmitEvent(RespawnCountChangedToEvent{.group = RespawnGroup::PLAYER2, .respawnCount = _respawnCount[2]});
}

void RespawnManager::OnBonusTank(const Author author)
{
	switch (author)
	{
		case Author::Enemy1:
		case Author::Enemy2:
		case Author::Enemy3:
		case Author::Enemy4:
			ChangeRespawnCount(1, RespawnGroup::ENEMY_ALL);
			break;
		case Author::Player1:
			ChangeRespawnCount(1, RespawnGroup::PLAYER1);
			break;
		case Author::Player2:
			ChangeRespawnCount(1, RespawnGroup::PLAYER2);
			break;
		case Author::None:
		case Author::lastId:
			break;
	}

	if (IsHost(_gameMode))
	{
		_events->EmitEvent(BonusTankAppliedEvent{.author = author});
	}
}

void RespawnManager::OnTankRespawned(const TankRespawnedEvent& event)
{
	const TankType type = event.type;
	switch (type)
	{
		case TankType::ENEMY1:
		case TankType::ENEMY2:
		case TankType::ENEMY3:
		case TankType::ENEMY4:
			ChangeRespawnCount(-1, RespawnGroup::ENEMY_ALL);
			break;
		case TankType::PLAYER1:
		case TankType::PLAYER2:
			ChangeRespawnCount(-1, type == TankType::PLAYER1
									   ? RespawnGroup::PLAYER1
									   : RespawnGroup::PLAYER2);
			break;
		case TankType::COOP1:
		case TankType::COOP2:
			break;
	}
}

void RespawnManager::OnTankSpawn(const TankSpawnEvent& event)
{
	const Uuid& uuid = event.uuid;
	if (const auto it = std::ranges::find(_slots, uuid, &SpawnSlot::uuid);
		it != _slots.end())
	{
		it->isAvailable = false;
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

bool RespawnManager::IsEnemyGroup(const RespawnGroup group)
{
	return group == RespawnGroup::ENEMY_ALL;
}

void RespawnManager::OnEnemyDied(const bool isAvailable)
{
	++_enemiesDeathCount;
	if (isAvailable == false && _enemiesSpawnCount == _enemiesDeathCount)
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
	}
}

void RespawnManager::OnTankDied(const TankDiedEvent& event)
{
	const Uuid& uuid = event.uuid;
	if (const auto it = std::ranges::find(_slots, uuid, &SpawnSlot::uuid);
		it != _slots.end())
	{
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
