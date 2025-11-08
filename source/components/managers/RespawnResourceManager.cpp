#include "components/managers/RespawnResourceManager.h"
#include "components/EventSystem.h"
#include "enums/GameMode.h"
#include "enums/RespawnResource.h"
#include "enums/TankType.h"
#include "utils/UuidUtils.h"
#include <algorithm>
#include <memory>
#include <boost/uuid/uuid.hpp>

//TODO: write spawn delay via timer separated for enemy and players team, example spawn every 5 sec one tank
RespawnResourceManager::RespawnResourceManager(const std::shared_ptr<EventSystem>& events) : _events{events}
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

RespawnResourceManager::~RespawnResourceManager()
{
	Unsubscribe();
}

void RespawnResourceManager::Subscribe()
{
	//TODO: reuse existing tanks when game mode changed
	//TODO: need work phase, clearState (all spawns disabled), battleState (spawn as normal)
	_events->AddListener("Reset", _name, [this]() { ResetSpawn(); });
	_events->AddListener("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_gameMode = newGameMode;

		_gameMode == GameMode::PlayAsClient ? SubscribeAsClient() : UnsubscribeAsClient();
	});

	_events->AddListener("TankSpawn", _name, [this](const buuid& uuid)
	{
		OnTankSpawn(uuid);
	});

	_events->AddListener("TankDied", _name, [this](const buuid& uuid)
	{
		OnTankDied(uuid);
	});

	_events->AddListener("BonusTank", _name, [this](const std::string& author, const std::string& fraction)
	{
		this->OnBonusTank(author, fraction);
	});
}

void RespawnResourceManager::SubscribeAsClient()
{
	_events->AddListener("ClientReceived_OnTank", _name, [this](const std::string& author, const std::string& fraction)
	{
		this->OnBonusTank(author, fraction);
	});

	_events->AddListener(
			"ClientReceived_OnGrenade", _name, [this](const std::string& author, const std::string& fraction)
			{
				this->OnBonusGrenade(author, fraction);
			});

	_events->AddListener("ClientReceived_RespawnTank", _name, [this](const TankType type, const buuid& /*uuid*/)
	{
		this->OnClientRespawn(type);
	});
}

void RespawnResourceManager::Unsubscribe() const
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
}

void RespawnResourceManager::UnsubscribeAsClient() const
{
	_events->RemoveListener("ClientReceived_OnTank", _name);
	_events->RemoveListener("ClientReceived_OnGrenade", _name);
	_events->RemoveListener("ClientReceived_RespawnTank", _name);
}

void RespawnResourceManager::SetEnemyNeedRespawn()
{
	for (size_t i = 0; i < 4; ++i)
	{
		_slots[i].isAvailable = true;
	}
}

//NOTE: use only in unit tests
void RespawnResourceManager::SetSlotNeedRespawn(const int slotIndex)
{
	if (slotIndex >= 0 && slotIndex < 6)
	{
		_slots[slotIndex].isAvailable = true;
	}
}

void RespawnResourceManager::ResetRespawnStat()
{
	_respawnResource[static_cast<int>(RespawnResource::ENEMY_ALL)] = 20;
	_respawnResource[static_cast<int>(RespawnResource::PLAYER_ONE)] = 3;
	_respawnResource[static_cast<int>(RespawnResource::PLAYER_TWO)] = 3;

	for (auto& [_, isAvailable]: _slots)
	{
		isAvailable = false;
	}
}

void RespawnResourceManager::ResetSpawn()
{
	ResetRespawnStat();

	SetPlayerNeedRespawn();

	SetEnemyNeedRespawn();
}

void RespawnResourceManager::SetPlayerNeedRespawn()
{
	_slots[4].isAvailable = true;

	if (_gameMode != GameMode::OnePlayer)
	{
		_slots[5].isAvailable = true;
	}
}

int RespawnResourceManager::GetEnemyRespawnResource() const
{
	return _respawnResource[static_cast<std::size_t>(RespawnResource::ENEMY_ALL)];
}

int RespawnResourceManager::GetPlayerOneRespawnResource() const
{
	return _respawnResource[static_cast<std::size_t>(RespawnResource::PLAYER_ONE)];
}

int RespawnResourceManager::GetPlayerTwoRespawnResource() const
{
	return _respawnResource[static_cast<std::size_t>(RespawnResource::PLAYER_TWO)];
}

std::string RespawnResourceManager::RespawnResourceEnumToString(const RespawnResource type)
{
	if (type == RespawnResource::ENEMY_ALL)
	{
		return std::string{"Enemy"};
	}

	if (type == RespawnResource::PLAYER_ONE)
	{
		return std::string{"Player1"};
	}

	return std::string{"Player2"};
}

void RespawnResourceManager::ChangeRespawnResource(const int delta, RespawnResource type)
{
	const auto id = static_cast<size_t>(type);
	_respawnResource[id] += delta;

	_events->EmitEvent("RespawnResourceChangedTo", RespawnResourceEnumToString(type), _respawnResource[id]);
}

void RespawnResourceManager::OnBonusGrenade(const std::string& author, const std::string& fraction)
{
	if (fraction == "PlayerTeam")
	{
		if (author == "Player1")
		{
			ChangeRespawnResource(-1, RespawnResource::PLAYER_ONE);
		}
		else if (author == "Player2")
		{
			ChangeRespawnResource(-1, RespawnResource::PLAYER_TWO);
		}
	}
	else if (fraction == "EnemyTeam")
	{
		ChangeRespawnResource(-1, RespawnResource::ENEMY_ALL);
	}
}

void RespawnResourceManager::OnBonusTank(const std::string& author, const std::string& fraction)
{
	if (fraction == "EnemyTeam")
	{
		ChangeRespawnResource(1, RespawnResource::ENEMY_ALL);
	}
	else if (fraction == "PlayerTeam")
	{
		if (author == "Player1" || author == "CoopBot1")
		{
			ChangeRespawnResource(1, RespawnResource::PLAYER_ONE);
		}
		else if (author == "Player2" || author == "CoopBot2")
		{
			ChangeRespawnResource(1, RespawnResource::PLAYER_TWO);
		}
	}

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent("ServerSend_OnTank", author, fraction);
	}
}

void RespawnResourceManager::OnClientRespawn(const TankType type)
{
	switch (type)
	{
		case TankType::ENEMY1:
		case TankType::ENEMY2:
		case TankType::ENEMY3:
		case TankType::ENEMY4:
			ChangeRespawnResource(-1, RespawnResource::ENEMY_ALL);
			break;
		case TankType::PLAYER1:
		case TankType::PLAYER2:
			ChangeRespawnResource(-1, type == TankType::PLAYER1
				                          ? RespawnResource::PLAYER_ONE
				                          : RespawnResource::PLAYER_TWO);
			break;
		default:
			break;
	}
}

void RespawnResourceManager::OnTankSpawn(const buuid& uuid)
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
					ChangeRespawnResource(-1, RespawnResource::ENEMY_ALL);
					break;
				case TankType::PLAYER1:
					ChangeRespawnResource(-1, RespawnResource::PLAYER_ONE);
					break;
				case TankType::PLAYER2:
					ChangeRespawnResource(-1, RespawnResource::PLAYER_TWO);
					break;
				default:
					break;
			}
			_slots[i].isAvailable = false;
			break;
		}
	}
}

void RespawnResourceManager::OnTankDied(const buuid& uuid)
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
					_slots[i].isAvailable = _respawnResource[static_cast<size_t>(RespawnResource::ENEMY_ALL)] > 0;
					break;
				case TankType::PLAYER1:
					_slots[i].isAvailable = _respawnResource[static_cast<size_t>(RespawnResource::PLAYER_ONE)] > 0;
					break;
				case TankType::PLAYER2:
					_slots[i].isAvailable = _respawnResource[static_cast<size_t>(RespawnResource::PLAYER_TWO)] > 0;
					break;
				default:
					break;
			}
			break;
		}
	}
}
