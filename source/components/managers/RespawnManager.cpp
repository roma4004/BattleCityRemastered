#include "components/managers/RespawnManager.h"
#include "components/EventSystem.h"
#include "enums/GameMode.h"
#include "enums/RespawnCount.h"
#include "enums/TankType.h"
#include "utils/UuidUtils.h"
#include <algorithm>
#include <boost/uuid/uuid.hpp>
#include <memory>

//TODO: write spawn delay via timer separated for enemy and players team, example spawn every 5 sec one tank
RespawnManager::RespawnManager(const std::shared_ptr<EventSystem>& events)
	: _events{events}
{
	_slots = {
			{.uuid = UuidUtils::GetRandomUuid(), .isAvailable = false},
			{.uuid = UuidUtils::GetRandomUuid(), .isAvailable = false},
			{.uuid = UuidUtils::GetRandomUuid(), .isAvailable = false},
			{.uuid = UuidUtils::GetRandomUuid(), .isAvailable = false},
			{.uuid = UuidUtils::GetRandomUuid(), .isAvailable = false},
			{.uuid = UuidUtils::GetRandomUuid(), .isAvailable = false},
	};

	Subscribe();
}

RespawnManager::~RespawnManager()
{
	Unsubscribe();
}

void RespawnManager::Subscribe()
{
	//TODO: reuse existing tanks when game mode changed
	//TODO: need work phase, clearState (all spawns disabled), battleState (spawn as normal)
	_events->AddListener("Reset", _name, [this]() { this->ResetSpawn(); });
	_events->AddListener(
			"GameModeChangedTo", _name,
			[this](const GameMode newGameMode)
			{
				this->_gameMode = newGameMode;

				_gameMode == GameMode::PlayAsClient ? SubscribeAsClient() : UnsubscribeAsClient();

				OnGameModeChange();
			});

	_events->AddListener("TankSpawn", _name, [this](const buuid& uuid) { OnTankSpawn(uuid); });

	_events->AddListener("TankDied", _name, [this](const buuid& uuid) { OnTankDied(uuid); });

	_events->AddListener("BonusTank", _name, [this](const std::string& author, const std::string& /*fraction*/)
	{
		this->OnBonusTank(author);
	});

	//NOTE: for unit tests
	_events->AddListener("SetSlotNeedRespawn", _name, [this](const int slotIndex)
	{
		this->SetSlotNeedRespawn(slotIndex);
	});

	_events->AddListener("PlayersBaseFinished", _name, [this]() { this->TriggerLastPlayersLife(); });
}

void RespawnManager::SubscribeAsClient()
{
	_events->AddListener("ClientReceived_OnTank", _name, [this](const std::string& author)
	{
		this->OnBonusTank(author);
	});

	_events->AddListener("ClientReceived_RespawnTank", _name, [this](const TankType type, const buuid& /*uuid*/)
	{
		this->OnClientRespawn(type);
	});
}

void RespawnManager::Unsubscribe() const
{
	_events->RemoveListener("Reset", _name);
	_events->RemoveListener("GameModeChangedTo", _name);
	_events->RemoveListener("TankSpawn", _name);
	_events->RemoveListener("TankDied", _name);

	if (_gameMode == GameMode::PlayAsClient)
	{
		UnsubscribeAsClient();
	}

	_events->RemoveListener("BonusTank", _name);
	_events->RemoveListener("SetSlotNeedRespawn", _name);
	_events->RemoveListener("PlayersBaseFinished", _name);
}

void RespawnManager::UnsubscribeAsClient() const
{
	_events->RemoveListener("ClientReceived_OnTank", _name);
	_events->RemoveListener("ClientReceived_RespawnTank", _name);
}

void RespawnManager::SetEnemyNeedRespawn()
{
	for (size_t i = 0; i < 4; ++i)
	{
		_slots[i].isAvailable = true;
	}
}

//NOTE: use only in unit tests
void RespawnManager::SetSlotNeedRespawn(const int slotIndex)
{
	if (slotIndex >= 0 && slotIndex < 6)
	{
		_slots[slotIndex].isAvailable = true;
	}
}

void RespawnManager::ResetRespawnStat()
{
	_respawnCount[static_cast<int>(RespawnCount::ENEMY_ALL)] = 20;
	_respawnCount[static_cast<int>(RespawnCount::PLAYER_ONE)] = 3;
	_respawnCount[static_cast<int>(RespawnCount::PLAYER_TWO)] = 3;

	for (auto& [_, isAvailable]: _slots)
	{
		isAvailable = false;
	}
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
	constexpr size_t player1Id = static_cast<size_t>(TankType::PLAYER1);
	_slots[player1Id].isAvailable = true;

	if (_gameMode != GameMode::OnePlayer)
	{
		constexpr size_t player2Id = static_cast<size_t>(TankType::PLAYER2);
		_slots[player2Id].isAvailable = true;
	}
}

int RespawnManager::GetEnemyRespawnCount() const
{
	return _respawnCount[static_cast<std::size_t>(RespawnCount::ENEMY_ALL)];
}

int RespawnManager::GetPlayerOneRespawnCount() const
{
	return _respawnCount[static_cast<std::size_t>(RespawnCount::PLAYER_ONE)];
}

int RespawnManager::GetPlayerTwoRespawnCount() const
{
	return _respawnCount[static_cast<std::size_t>(RespawnCount::PLAYER_TWO)];
}

std::string RespawnManager::RespawnCountEnumToString(const RespawnCount type)
{
	if (type == RespawnCount::ENEMY_ALL)
	{
		return std::string{"Enemy"};
	}

	if (type == RespawnCount::PLAYER_ONE)
	{
		return std::string{"Player1"};
	}

	return std::string{"Player2"};
}

void RespawnManager::ChangeRespawnCount(const int delta, RespawnCount type)
{
	const auto id = static_cast<size_t>(type);
	_respawnCount[id] += delta;

	_events->EmitEvent("RespawnCountChangedTo", RespawnCountEnumToString(type), _respawnCount[id]);
}

void RespawnManager::TriggerLastPlayersLife()
{
	_respawnCount[1] = 0;
	_respawnCount[2] = 0;
}

void RespawnManager::OnBonusTank(const std::string& author)
{
	if (author.starts_with("Enemy"))
	{
		ChangeRespawnCount(1, RespawnCount::ENEMY_ALL);
	}
	else if (author.ends_with("1"))
	{
		ChangeRespawnCount(1, RespawnCount::PLAYER_ONE);
	}
	else if (author.ends_with("2"))
	{
		ChangeRespawnCount(1, RespawnCount::PLAYER_TWO);
	}

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent("ServerSend_OnTank", author);
	}
}

void RespawnManager::OnClientRespawn(const TankType type)
{
	switch (type)
	{
		case TankType::ENEMY1:
		case TankType::ENEMY2:
		case TankType::ENEMY3:
		case TankType::ENEMY4:
			ChangeRespawnCount(-1, RespawnCount::ENEMY_ALL);
			break;
		case TankType::PLAYER1:
		// case TankType::COOP1:
		case TankType::PLAYER2:
		// case TankType::COOP2:
			ChangeRespawnCount(-1, type == TankType::PLAYER1
									   ? RespawnCount::PLAYER_ONE
									   : RespawnCount::PLAYER_TWO);
			break;
		default:
			break;
	}
}

void RespawnManager::OnTankSpawn(const buuid& uuid)
{
	for (size_t i = 0u; i < _slots.size(); ++i)
	{
		if (_slots[i].uuid == uuid)
		{
			switch (static_cast<TankType>(i))
			{
				case TankType::ENEMY1:
				case TankType::ENEMY2:
				case TankType::ENEMY3:
				case TankType::ENEMY4:
					ChangeRespawnCount(-1, RespawnCount::ENEMY_ALL);
					++_enemiesSpawnCount;
					break;
				case TankType::PLAYER1:
				case TankType::COOP1:
					ChangeRespawnCount(-1, RespawnCount::PLAYER_ONE);
					++_playersSpawnCount;
					break;
				case TankType::PLAYER2:
				case TankType::COOP2:
					ChangeRespawnCount(-1, RespawnCount::PLAYER_TWO);
					++_playersSpawnCount;
					break;
				default:
					break;
			}
			_slots[i].isAvailable = false;
			break;
		}
	}
}

void RespawnManager::EnemyDied(const bool isAvailable)
{
	++_enemiesDeathCount;
	if (isAvailable == false && _enemiesSpawnCount == _enemiesDeathCount)
	{
		_events->EmitEvent("PlayersTeamIsWon");
	}
}

void RespawnManager::PlayerDied(const bool isAvailable)
{
	++_playersDeathCount;
	if (isAvailable == false && _playersSpawnCount == _playersDeathCount)
	{
		_events->EmitEvent("EnemiesTeamIsWon");
	}
}

void RespawnManager::OnTankDied(const buuid& uuid)
{
	//TODO: replace with std:: algorithm
	for (size_t i = 0u; i < _slots.size(); ++i)
	{
		if (_slots[i].uuid == uuid)
		{
			switch (static_cast<TankType>(i))
			{
				case TankType::ENEMY1:
				case TankType::ENEMY2:
				case TankType::ENEMY3:
				case TankType::ENEMY4:
					_slots[i].isAvailable = _respawnCount[static_cast<size_t>(RespawnCount::ENEMY_ALL)] > 0;
					EnemyDied(_slots[i].isAvailable);
					break;
				case TankType::PLAYER1:
				case TankType::COOP1:
					_slots[i].isAvailable = _respawnCount[static_cast<size_t>(RespawnCount::PLAYER_ONE)] > 0;
					PlayerDied(_slots[i].isAvailable);
					break;
				case TankType::PLAYER2:
				case TankType::COOP2:
					_slots[i].isAvailable = _respawnCount[static_cast<size_t>(RespawnCount::PLAYER_TWO)] > 0;
					PlayerDied(_slots[i].isAvailable);
					break;
				default:
					break;
			}
			break;
		}
	}
}
