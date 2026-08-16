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
#include <boost/uuid/uuid.hpp>
#include <memory>

RespawnManager::RespawnManager(const std::shared_ptr<EventSystem>& events)
	: _events{events}
{
	_slots = {
			{.uuid = UuidUtils::GetRandomUuid(), .type = TankType::ENEMY1, .group = RespawnGroup::ENEMY_ALL},
			{.uuid = UuidUtils::GetRandomUuid(), .type = TankType::ENEMY2, .group = RespawnGroup::ENEMY_ALL},
			{.uuid = UuidUtils::GetRandomUuid(), .type = TankType::ENEMY3, .group = RespawnGroup::ENEMY_ALL},
			{.uuid = UuidUtils::GetRandomUuid(), .type = TankType::ENEMY4, .group = RespawnGroup::ENEMY_ALL},
			{.uuid = UuidUtils::GetRandomUuid(), .type = TankType::PLAYER1, .group = RespawnGroup::PLAYER_ONE},
			{.uuid = UuidUtils::GetRandomUuid(), .type = TankType::PLAYER2, .group = RespawnGroup::PLAYER_TWO},
	};

	Subscribe();
}

void RespawnManager::Subscribe()
{
	//TODO: reuse existing tanks when game mode changed
	//TODO: need work phase, clearState (all spawns disabled), battleState (spawn as normal)
	_subs.push_back(_events->AddListener(this, &RespawnManager::OnGameReset));
	_subs.push_back(_events->AddListener(this, &RespawnManager::OnGameModeChangedTo));
	_subs.push_back(_events->AddListener(this, &RespawnManager::OnTankSpawn));
	_subs.push_back(_events->AddListener(this, &RespawnManager::OnTankDied));
	_subs.push_back(_events->AddListener(this, &RespawnManager::OnBonusTankPickup));
	_subs.push_back(_events->AddListener(this, &RespawnManager::OnPlayersBaseFinished));
	_subs.push_back(_events->AddListener(this, &RespawnManager::OnRespawnTanks));
}

void RespawnManager::OnGameReset(const GameResetEvent&) { ResetSpawn(); }

void RespawnManager::OnGameModeChangedTo(const GameModeChangedToEvent& event)
{
	_gameMode = event.mode;

	_gameMode == GameMode::PlayAsClient ? SubscribeAsClient() : UnsubscribeAsClient();

	OnGameModeChange();
}

void RespawnManager::OnBonusTankPickup(const BonusTankPickupEvent& event) { OnBonusTank(event.author); }

void RespawnManager::OnPlayersBaseFinished(const PlayersBaseFinishedEvent&) { TriggerLastPlayersLife(); }

void RespawnManager::OnRespawnTanks(const RespawnTanksEvent& event) { RespawnTanks(event.skipDelay); }

void RespawnManager::SubscribeAsClient()
{
	_clientSubs.push_back(_events->AddListener(this, &RespawnManager::OnClientInBonusTankPickup));
	_clientSubs.push_back(_events->AddListener(this, &RespawnManager::OnClientRespawn));
}

void RespawnManager::OnClientInBonusTankPickup(const ClientInBonusTankPickupEvent& event) { OnBonusTank(event.name); }

void RespawnManager::UnsubscribeAsClient() { _clientSubs.clear(); }

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
	_respawnCount[static_cast<int>(RespawnGroup::PLAYER_ONE)] = 3u;
	_respawnCount[static_cast<int>(RespawnGroup::PLAYER_TWO)] = 3u;

	for (auto& [uuid, tankType, respawnGroup, isAvailable]: _slots)
	{
		isAvailable = false;
	}

	_enemiesSpawnCount = 0u;
	_enemiesDeathCount = 0u;
	_playersSpawnCount = 0u;
	_playersDeathCount = 0u;
}

void RespawnManager::ResetSpawn()
{
	ResetRespawnStat();
}

void RespawnManager::OnGameModeChange()
{
	SetPlayerNeedRespawn();

	SetEnemyNeedRespawn();
}

void RespawnManager::SetPlayerNeedRespawn()
{
	constexpr auto player1Id = static_cast<size_t>(TankType::PLAYER1);
	_slots[player1Id].isAvailable = true;

	if (_gameMode != GameMode::OnePlayer)
	{
		constexpr auto player2Id = static_cast<size_t>(TankType::PLAYER2);
		_slots[player2Id].isAvailable = true;
	}
}

std::string RespawnManager::RespawnCountEnumToString(const RespawnGroup type)
{
	if (type == RespawnGroup::ENEMY_ALL)
	{
		return std::string{"Enemy"};
	}

	if (type == RespawnGroup::PLAYER_ONE)
	{
		return std::string{"Player1"};
	}

	return std::string{"Player2"};
}

void RespawnManager::ChangeRespawnCount(const int delta, RespawnGroup type)
{
	const auto id = static_cast<size_t>(type);
	if (const int newCount = _respawnCount[id] + delta; newCount >= 0)
	{
		_respawnCount[id] = static_cast<unsigned short>(newCount);
	}

	const std::string who = RespawnCountEnumToString(type);
	_events->EmitEvent(RespawnCountChangedToEvent{.objectName = who, .respawnCount = _respawnCount[id]});
}

void RespawnManager::TriggerLastPlayersLife()
{
	_respawnCount[1] = 0u;
	_events->EmitEvent(RespawnCountChangedToEvent{.objectName = "Player1", .respawnCount = _respawnCount[1]});
	_respawnCount[2] = 0u;
	_events->EmitEvent(RespawnCountChangedToEvent{.objectName = "Player2", .respawnCount = _respawnCount[2]});
}

void RespawnManager::OnBonusTank(const std::string& author)
{
	if (author.starts_with("Enemy"))
	{
		ChangeRespawnCount(1, RespawnGroup::ENEMY_ALL);
	}
	else if (author.ends_with("1"))
	{
		ChangeRespawnCount(1, RespawnGroup::PLAYER_ONE);
	}
	else if (author.ends_with("2"))
	{
		ChangeRespawnCount(1, RespawnGroup::PLAYER_TWO);
	}

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent(ServerOutBonusTankPickupEvent{.author = author});
	}
}

void RespawnManager::OnClientRespawn(const ClientInRespawnTankEvent& event)
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
									   ? RespawnGroup::PLAYER_ONE
									   : RespawnGroup::PLAYER_TWO);
			break;
		case TankType::COOP1:
		case TankType::COOP2:
			break;
	}
}

void RespawnManager::OnTankSpawn(const TankSpawnEvent& event)
{
	const buuid& uuid = event.uuid;
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
		_events->EmitEvent(PlayersTeamIsWonEvent{});
		if (_gameMode == GameMode::PlayAsHost)
		{
			_events->EmitEvent(ServerOutPlayersTeamIsWonEvent{});
		}
	}
}

void RespawnManager::OnPlayerDied(const bool isAvailable)
{
	++_playersDeathCount;
	if (isAvailable == false && _playersSpawnCount == _playersDeathCount)
	{
		_events->EmitEvent(EnemiesTeamIsWonEvent{});
		if (_gameMode == GameMode::PlayAsHost)
		{
			_events->EmitEvent(ServerOutEnemiesTeamIsWonEvent{});
		}
	}
}

void RespawnManager::OnTankDied(const TankDiedEvent& event)
{
	const buuid& uuid = event.uuid;
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

void RespawnManager::RespawnTanks(const bool skipDelay)
{
	for (const auto& slot: _slots | std::ranges::views::filter([](const auto& s) { return s.isAvailable; }))
	{
		_events->EmitEvent(RespawnTankEvent{.type = slot.type, .uuid = slot.uuid, .skipDelay = skipDelay});
	}
}
