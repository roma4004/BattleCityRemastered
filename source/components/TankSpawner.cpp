#include "components/TankSpawner.h"
#include "components/BonusEffectManager.h"
#include "components/EventSystem.h"
#include "components/input/InputProviderForPlayerOne.h"
#include "components/input/InputProviderForPlayerOneNet.h"
#include "components/input/InputProviderForPlayerTwo.h"
#include "components/input/InputProviderForPlayerTwoNet.h"
#include "entities/pawns/CoopBot.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/Player.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/RespawnResource.h"
#include "enums/TankType.h"
#include "utils/ColliderUtils.h"
#include "utils/Logger.h"
#include "utils/UuidUtils.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <boost/uuid/uuid.hpp>

//TODO: fix respawn on client
TankSpawner::TankSpawner(const UPoint windowSize, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                         std::shared_ptr<EventSystem> events, std::shared_ptr<BulletPool> bulletPool,
                         std::shared_ptr<BonusEffectManager> bonusEffectManager)
	: _windowSize{windowSize},
	  _allObjects{allObjects},
	  _events{std::move(events)},
	  _bulletPool{std::move(bulletPool)},
	  _bonusEffectManager{std::move(bonusEffectManager)}
{
	_slots = {
			{UuidUtils::GetRandomUuid(), false},
			{UuidUtils::GetRandomUuid(), false},
			{UuidUtils::GetRandomUuid(), false},
			{UuidUtils::GetRandomUuid(), false},
			{UuidUtils::GetRandomUuid(), false},
			{UuidUtils::GetRandomUuid(), false},
	};

	Subscribe();
}

TankSpawner::~TankSpawner()
{
	Unsubscribe();
}

void TankSpawner::Subscribe()
{
	//TODO: reuse existing tanks when game mode changed
	//TODO: need work phase, clearState (all spawns disabled), battleState (spawn as normal)
	_events->AddListener("Reset", _name, [this]() { ResetSpawn(); });
	_events->AddListener<const GameMode>("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_gameMode = newGameMode;

		_gameMode == PlayAsClient ? SubscribeAsClient() : UnsubscribeAsClient();
	});
	_events->AddListener<const buuid&>("TankSpawn", _name, [this](const buuid& uuid)
	{
		OnTankSpawn(uuid);
	});
	_events->AddListener<const buuid&>("TankDied", _name, [this](const buuid& uuid)
	{
		OnTankDied(uuid);
	});

	_events->AddListener<const std::string&, const std::string&>(
			"BonusTank", _name, [this](const std::string& author, const std::string& fraction)
			{
				this->OnBonusTank(author, fraction);
			});
}

void TankSpawner::SubscribeAsClient()
{
	_events->AddListener<const std::string&, const std::string&>(
			"ClientReceived_OnTank", _name, [this](const std::string& author, const std::string& fraction)
			{
				this->OnBonusTank(author, fraction);
			});

	_events->AddListener<const std::string&, const std::string&>(
			"ClientReceived_OnGrenade", _name, [this](const std::string& author, const std::string& fraction)
			{
				this->OnBonusGrenade(author, fraction);
			});

	_events->AddListener<const TankType, const buuid&>(
			"ClientReceived_RespawnTank", _name, [this](const TankType type, const buuid& uuid)
			{
				this->RespawnClient(type, uuid);
			});
}

void TankSpawner::Unsubscribe() const
{
	_events->RemoveListener("Reset", _name);
	_events->RemoveListener<const GameMode>("GameModeChangedTo", _name);
	_events->RemoveListener<const buuid&>("TankSpawn", _name);
	_events->RemoveListener<const buuid&>("TankDied", _name);

	if (_gameMode == PlayAsClient)
	{
		UnsubscribeAsClient();
	}

	_events->RemoveListener<const std::string&, const std::string&>("BonusTank", _name);
}

void TankSpawner::UnsubscribeAsClient() const
{
	_events->RemoveListener<const std::string&, const std::string&>("ClientReceived_" + _name + "OnTank", _name);
	_events->RemoveListener<const std::string&, const std::string&>("ClientReceived_OnGrenade", _name);
}

void TankSpawner::SetEnemyNeedRespawn()
{
	for (int i = 0; i < 4; ++i)
	{
		_slots[i].isAvailable = true;
	}
}

//NOTE: use only in unit tests
void TankSpawner::SetSlotNeedRespawn(const int slotIndex)
{
	if (slotIndex >= 0 && slotIndex < 6)
	{
		_slots[slotIndex].isAvailable = true;
	}
}

void TankSpawner::ResetRespawnStat()
{
	_respawnResource[ENEMY_ALL] = 20;
	_respawnResource[PLAYER_ONE] = 3;
	_respawnResource[PLAYER_TWO] = 3;

	for (SpawnSlot& slot: _slots)
	{
		slot.isAvailable = false;
	}
}

void TankSpawner::ResetSpawn()
{
	ResetRespawnStat();

	SetPlayerNeedRespawn();

	SetEnemyNeedRespawn();
}

void TankSpawner::SetPlayerNeedRespawn()
{
	_slots[4].isAvailable = true;

	if (_gameMode != OnePlayer)
	{
		_slots[5].isAvailable = true;
	}
}

std::string TankSpawner::GetCurrentTimeString()
{
	const auto now = std::chrono::system_clock::now();
	const auto nowTime = std::chrono::system_clock::to_time_t(now);
	const auto ms = std::chrono::duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

	std::tm timeInfo;
	localtime_s(&timeInfo, &nowTime);

	std::stringstream ss;
	ss << std::put_time(&timeInfo, "%H:%M:%S") << '.'
			<< std::setfill('0') << std::setw(3) << ms.count();

	return ss.str();
}

void TankSpawner::SpawnEnemy(const buuid uuid, const TankType type, const float speed, const int health)
{
	const float gridOffset{static_cast<float>(_windowSize.y) / 50.f};
	const float size{gridOffset * 3};
	const static std::vector<ObjRectangle> spawnPos{{
			{.x = gridOffset * 16.f - size * 2.f, .y = 0, .w = size, .h = size},
			{.x = gridOffset * 32.f - size * 2.f, .y = 0, .w = size, .h = size},
			{.x = gridOffset * 16.f + size * 2.f, .y = 0, .w = size, .h = size},
			{.x = gridOffset * 32.f + size * 2.f, .y = 0, .w = size, .h = size}
	}};

	for (const auto& rect: spawnPos)
	{
		const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [&rect](const std::shared_ptr<BaseObj>& object)
		{
			if (object.get() == nullptr)
			{
				return false;
			}

			return ColliderUtils::IsCollide(rect, object->GetRect());
		});

		if (isFreeSpawnSpot)
		{
			std::string fraction{"EnemyTeam"};
			std::string name{"Enemy" + std::to_string(type + 1)};

			// Log enemy tank spawn
			const std::string uuidString = UuidUtils::GetStringUuid(uuid);
			Logger::GetInstance().LogTankSpawn(name, fraction, uuidString);
			std::cout << "[" << GetCurrentTimeString() << "] "
					<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
					<< "SpawnEnemy  UUID = " << uuidString
					<< ", Name = " << name
					<< std::endl;

			constexpr int gray{0x808080};
			const bool isTimerActive = _bonusEffectManager->GetTimerEnemy().isActive;
			const bool isHelmetActive = _bonusEffectManager->GetHelmet(type).isActive;
			SpawnTank(rect, gray, health, std::move(name), std::move(fraction), speed, std::move(uuid),
			          {isTimerActive, isHelmetActive}, type);
			return;
		}
	}
}

void TankSpawner::SpawnPlayer(ObjRectangle rect, const float speed, const int health, const buuid uuid,
                              const TankType type)
{
	const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [&rect](const std::shared_ptr<BaseObj>& object)
	{
		if (object.get() == nullptr)
		{
			return false;
		}

		return ColliderUtils::IsCollide(rect, object->GetRect());
	});

	if (isFreeSpawnSpot)
	{
		std::string name{type == PLAYER1 ? "Player1" : "Player2"};
		std::string fraction{"PlayerTeam"};

		// Log tank spawn
		const std::string uuidString = UuidUtils::GetStringUuid(uuid);
		Logger::GetInstance().LogTankSpawn(name, fraction, uuidString);
		std::cout << "[" << GetCurrentTimeString() << "] "
				<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
				<< "SpawnPlayer UUID = " << uuidString
				<< ", Name = " << name
				<< std::endl;

		constexpr int yellow{0xeaea00};
		constexpr int green{0x408000};
		const int color = type == PLAYER1 ? yellow : green;
		const bool isTimerActive = _bonusEffectManager->GetTimerPlayer().isActive;
		const bool isHelmetActive = _bonusEffectManager->GetHelmet(type == PLAYER1 ? 4 : 5).isActive;
		SpawnTank(rect, color, health, std::move(name), std::move(fraction), speed, std::move(uuid),
		          {isTimerActive, isHelmetActive}, type);
	}
}

void TankSpawner::SpawnCoopBot(ObjRectangle rect, const float speed, const int health, const buuid uuid,
                               const TankType type)
{
	const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [&rect](const std::shared_ptr<BaseObj>& object)
	{
		if (object.get() == nullptr)
		{
			return false;
		}

		return ColliderUtils::IsCollide(rect, object->GetRect());
	});

	if (isFreeSpawnSpot)
	{
		std::string name{(type == COOP1 ? "CoopBot1" : "CoopBot2")};
		std::string fraction{"PlayerTeam"};

		// Log coop bot spawn
		const std::string uuidString = UuidUtils::GetStringUuid(uuid);
		Logger::GetInstance().LogTankSpawn(name, fraction, uuidString);
		std::cout << "[" << GetCurrentTimeString() << "] "
				<< "[" << (_gameMode == PlayAsHost ? "SERVER" : "CLIENT") << "] "
				<< "SpawnEnemy  UUID = " << uuidString
				<< ", Name = " << name
				<< std::endl;

		constexpr int yellow{0xeaea00};
		constexpr int green{0x408000};
		const int color = type == COOP1 ? yellow : green;
		const bool isTimerActive = _bonusEffectManager->GetTimerPlayer().isActive;
		const bool isHelmetActive = _bonusEffectManager->GetHelmet(type == COOP1 ? 4 : 5).isActive;
		SpawnTank(rect, color, health, std::move(name), std::move(fraction), speed, std::move(uuid),
		          {isTimerActive, isHelmetActive}, type);
	}
}

void TankSpawner::RespawnEnemyTanks(const TankType type, const buuid uuid)
{
	constexpr float speed{142};
	constexpr int health{100};
	SpawnEnemy(uuid, type, speed, health);

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const TankType, const buuid&>("ServerSend_RespawnTank", type, uuid);
	}
}

void TankSpawner::RespawnPlayerTeam(const TankType type, const buuid uuid)
{
	const float windowSizeY{static_cast<float>(_windowSize.y)};
	const float gridOffset{windowSizeY / 50.f};
	const float size{gridOffset * 3};
	constexpr float speed{142};
	constexpr int health{100};
	const bool isFirst = type == PLAYER1;
	const ObjRectangle rect{.x = gridOffset * (isFirst ? 16.f : 32.f), .y = windowSizeY - size, .w = size, .h = size};

	if (_gameMode == OnePlayer
	    || _gameMode == TwoPlayers
	    || _gameMode == PlayAsHost
	    || _gameMode == PlayAsClient
	    || _gameMode == CoopWithBot && isFirst)
	{
		SpawnPlayer(rect, speed, health, uuid, type);
	}
	else if (_gameMode == Demo || _gameMode == CoopWithBot)
	{
		SpawnCoopBot(rect, speed, health, uuid, isFirst ? COOP1 : COOP2);
	}

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const TankType, const buuid&>("ServerSend_RespawnTank", type, uuid);
	}
}

void TankSpawner::RespawnTanks()
{
	for (int i = 0; i < _slots.size(); ++i)
	{
		if (_slots[i].isAvailable)
		{
			switch (i)
			{
				case TankType::ENEMY1:
				case TankType::ENEMY2:
				case TankType::ENEMY3:
				case TankType::ENEMY4:
					RespawnEnemyTanks(static_cast<TankType>(i), _slots[i].id);
					break;
				case TankType::PLAYER1:
				case TankType::PLAYER2:
					RespawnPlayerTeam(static_cast<TankType>(i), _slots[i].id);
					break;
				default:
					break;
			}
			_slots[i].isAvailable = false;
			break;
		}
	}
}

void TankSpawner::RespawnClient(const TankType type, const buuid uuid)
{
	switch (type)
	{
		case TankType::ENEMY1:
		case TankType::ENEMY2:
		case TankType::ENEMY3:
		case TankType::ENEMY4:
			RespawnEnemyTanks(type, uuid);
			break;
		case TankType::PLAYER1:
		case TankType::PLAYER2:
			RespawnPlayerTeam(type, uuid);
			break;
		default:
			break;
	}
}

void TankSpawner::IncreaseEnemyRespawnResource()
{
	++_respawnResource[ENEMY_ALL];
	_events->EmitEvent<const std::string&, const int>("RespawnResourceChangedTo", "Enemy",
	                                                  _respawnResource[ENEMY_ALL]);
}

void TankSpawner::IncreasePlayerOneRespawnResource()//TODO: combine increase for both players into one
{
	++_respawnResource[PLAYER_ONE];
	_events->EmitEvent<const std::string&, const int>("RespawnResourceChangedTo", "Player1",
	                                                  _respawnResource[PLAYER_ONE]);
}

void TankSpawner::IncreasePlayerTwoRespawnResource()
{
	++_respawnResource[PLAYER_TWO];
	_events->EmitEvent<const std::string&, const int>("RespawnResourceChangedTo", "Player2",
	                                                  _respawnResource[PLAYER_TWO]);
}

void TankSpawner::DecreaseEnemyRespawnResource()
{
	--_respawnResource[ENEMY_ALL];
	_events->EmitEvent<const std::string&, const int>("RespawnResourceChangedTo", "Enemy",
	                                                  _respawnResource[ENEMY_ALL]);
}

void TankSpawner::DecreasePlayerOneRespawnResource()
{
	--_respawnResource[PLAYER_ONE];
	_events->EmitEvent<const std::string&, const int>("RespawnResourceChangedTo", "Player1",
	                                                  _respawnResource[PLAYER_ONE]);
}

void TankSpawner::DecreasePlayerTwoRespawnResource()
{
	--_respawnResource[PLAYER_TWO];
	_events->EmitEvent<const std::string&, const int>("RespawnResourceChangedTo", "Player2",
	                                                  _respawnResource[PLAYER_TWO]);
}

void TankSpawner::OnBonusGrenade(const std::string& author, const std::string& fraction)
{
	if (fraction == "PlayerTeam")
	{
		if (author == "Player1")
		{
			DecreasePlayerOneRespawnResource();
		}
		else if (author == "Player2")
		{
			DecreasePlayerTwoRespawnResource();
		}
	}
	else if (fraction == "EnemyTeam")
	{
		DecreaseEnemyRespawnResource();
	}
}

void TankSpawner::OnBonusTank(const std::string& author, const std::string& fraction)
{
	if (fraction == "EnemyTeam")
	{
		IncreaseEnemyRespawnResource();
	}
	else if (fraction == "PlayerTeam")
	{
		if (author == "Player1" || author == "CoopBot1")
		{
			IncreasePlayerOneRespawnResource();
		}
		else if (author == "Player2" || author == "CoopBot2")
		{
			IncreasePlayerTwoRespawnResource();
		}
	}

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const std::string&>("ServerSend_OnTank", author, fraction);
	}
}

void TankSpawner::OnTankSpawn(const buuid& uuid)
{
	for (int i = 0; i < _slots.size(); ++i)
	{
		if (_slots[i].id == uuid)
		{
			switch (i)
			{
				case TankType::ENEMY1:
				case TankType::ENEMY2:
				case TankType::ENEMY3:
				case TankType::ENEMY4:
					DecreaseEnemyRespawnResource();
					break;
				case TankType::PLAYER1:
					DecreasePlayerOneRespawnResource();
					break;
				case TankType::PLAYER2:
					DecreasePlayerTwoRespawnResource();
					break;
				default:
					break;
			}
			_slots[i].isAvailable = false;
			break;
		}
	}
}

void TankSpawner::OnTankDied(const buuid& uuid)
{
	//TODO: replace with std:: algorithm
	for (int i = 0; i < _slots.size(); ++i)
	{
		if (_slots[i].id == uuid)
		{
			switch (i)
			{
				case TankType::ENEMY1:
				case TankType::ENEMY2:
				case TankType::ENEMY3:
				case TankType::ENEMY4:
					_slots[i].isAvailable = _respawnResource[ENEMY_ALL] > 0;
					break;
				case TankType::PLAYER1:
					_slots[i].isAvailable = _respawnResource[PLAYER_ONE] > 0;
					break;
				case TankType::PLAYER2:
					_slots[i].isAvailable = _respawnResource[PLAYER_TWO] > 0;
					break;
				default:
					break;
			}
			break;
		}
	}
}

std::unique_ptr<IInputProvider> TankSpawner::GetInputProvider(const TankType type)
{
	if (type == PLAYER1)
	{
		if (_gameMode == PlayAsClient)
		{
			return std::make_unique<InputProviderForPlayerOneNet>(_events);
		}

		return std::make_unique<InputProviderForPlayerOne>(_events);
	}

	if (_gameMode == PlayAsClient || _gameMode == PlayAsHost)
	{
		return std::make_unique<InputProviderForPlayerTwoNet>(_events);
	}

	return std::make_unique<InputProviderForPlayerTwo>(_events);
}

std::shared_ptr<BaseObj> TankSpawner::CreateTank(const TankType type, PawnProperty pawnProperty,
                                                 BonusEffectProperty effects)
{
	if (type == ENEMY1 || type == ENEMY2 || type == ENEMY3 || type == ENEMY4)
	{
		return std::make_shared<Enemy>(std::move(pawnProperty), _bulletPool, std::move(effects));
	}

	if (type == COOP1 || type == COOP2)
	{
		return std::make_shared<CoopBot>(std::move(pawnProperty), _bulletPool, std::move(effects));
	}

	return std::make_shared<Player>(
			std::move(pawnProperty), _bulletPool, std::move(GetInputProvider(type)), std::move(effects));
}

void TankSpawner::SpawnTank(const ObjRectangle rect, int color, int health, std::string name, std::string fraction,
                            const float speed, buuid uuid, BonusEffectProperty effects, const TankType type)
{
	BaseObjProperty baseObjProperty{rect, color, health, uuid, std::move(name), std::move(fraction)};
	PawnProperty pawnProperty{
			std::move(baseObjProperty), _allObjects, _events, 1, speed, _windowSize, UP, _gameMode};

	if (std::shared_ptr<BaseObj> tank{CreateTank(type, std::move(pawnProperty), std::move(effects))})
	{
		_allObjects->emplace_back(tank);
	}
}
