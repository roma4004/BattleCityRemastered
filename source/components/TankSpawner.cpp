#include "components/TankSpawner.h"
#include "components/EventSystem.h"
#include "components/input/InputProviderForPlayerOne.h"
#include "components/input/InputProviderForPlayerOneNet.h"
#include "components/input/InputProviderForPlayerTwo.h"
#include "components/input/InputProviderForPlayerTwoNet.h"
#include "components/managers/BonusEffectManager.h"
#include "entities/pawns/CoopBot.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/Player.h"
#include "enums/AnimationType.h"
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

//TODO: write spawn delay via timer separated for enemy and players team, example spawn every 5 sec one tank
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
			{.uuid = UuidUtils::GetRandomUuid(), .isAvailable = false},
			{.uuid = UuidUtils::GetRandomUuid(), .isAvailable = false},
			{.uuid = UuidUtils::GetRandomUuid(), .isAvailable = false},
			{.uuid = UuidUtils::GetRandomUuid(), .isAvailable = false},
			{.uuid = UuidUtils::GetRandomUuid(), .isAvailable = false},
			{.uuid = UuidUtils::GetRandomUuid(), .isAvailable = false},
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

	_events->AddListener("SpawnEnabled", _name, [this](std::weak_ptr<Tank> tank)
	{
		_events->EmitEvent("AnimationCreateTank", tank);
	});
}

void TankSpawner::SubscribeAsClient()
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

	_events->AddListener("ClientReceived_RespawnTank", _name, [this](const TankType type, const buuid& uuid)
	{
		constexpr bool skipDelay = true;
		this->OnClientRespawn(type, uuid, skipDelay);
	});
}

void TankSpawner::Unsubscribe() const
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

void TankSpawner::UnsubscribeAsClient() const
{
	_events->RemoveListener("ClientReceived_OnTank", _name);
	_events->RemoveListener("ClientReceived_OnGrenade", _name);
	_events->RemoveListener("ClientReceived_RespawnTank", _name);
}

void TankSpawner::SetEnemyNeedRespawn()
{
	for (size_t i = 0; i < 4; ++i)
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
	_respawnResource[static_cast<int>(RespawnResource::ENEMY_ALL)] = 20;
	_respawnResource[static_cast<int>(RespawnResource::PLAYER_ONE)] = 3;
	_respawnResource[static_cast<int>(RespawnResource::PLAYER_TWO)] = 3;

	for (auto& [_, isAvailable]: _slots)
	{
		isAvailable = false;
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

	if (_gameMode != GameMode::OnePlayer)
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

void TankSpawner::SpawnEnemy(const buuid uuid, const TankType type, const float speed, const int health,
                             const bool skipDelay)
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
			if (object == nullptr)
			{
				return false;
			}

			return ColliderUtils::IsCollide(rect, object->GetRect());
		});

		if (isFreeSpawnSpot)
		{
			std::string fraction{"EnemyTeam"};
			std::string name{"Enemy" + std::to_string(static_cast<int>(type) + 1)};

			// Log enemy tank spawn
			const std::string uuidString = UuidUtils::GetStringUuid(uuid);
			Logger::GetInstance().LogTankSpawn(name, fraction, uuidString);
			std::cout << "[" << GetCurrentTimeString() << "] "
					<< "[" << (_gameMode == GameMode::PlayAsHost ? "SERVER" : "CLIENT") << "] "
					<< "SpawnEnemy  UUID = " << uuidString
					<< ", Name = " << name
					<< std::endl;

			constexpr int gray{0x808080};
			const bool isTimerActive = _bonusEffectManager->GetTimerEnemy().isActive;
			const bool isHelmetActive = _bonusEffectManager->GetHelmet(static_cast<int>(type)).isActive;
			const BonusEffectProperty effects = {isTimerActive, isHelmetActive};

			SpawnTank(rect, gray, health, std::move(name), std::move(fraction), speed, std::move(uuid), effects, type,
			          skipDelay);

			return;
		}
	}
}

void TankSpawner::SpawnPlayer(ObjRectangle rect, const float speed, const int health, const buuid uuid,
                              const TankType type, const bool skipDelay)
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
		std::string name{type == TankType::PLAYER1 ? "Player1" : "Player2"};
		std::string fraction{"PlayerTeam"};

		// Log tank spawn
		const std::string uuidString = UuidUtils::GetStringUuid(uuid);
		Logger::GetInstance().LogTankSpawn(name, fraction, uuidString);
		std::cout << "[" << GetCurrentTimeString() << "] "
				<< "[" << (_gameMode == GameMode::PlayAsHost ? "SERVER" : "CLIENT") << "] "
				<< "SpawnPlayer UUID = " << uuidString
				<< ", Name = " << name
				<< std::endl;

		constexpr int yellow{0xeaea00};
		constexpr int green{0x408000};
		const int color = type == TankType::PLAYER1 ? yellow : green;
		const bool isTimerActive = _bonusEffectManager->GetTimerPlayer().isActive;
		const bool isHelmetActive = _bonusEffectManager->GetHelmet(type == TankType::PLAYER1 ? 4 : 5).isActive;
		const BonusEffectProperty effects = {isTimerActive, isHelmetActive};

		SpawnTank(rect, color, health, std::move(name), std::move(fraction), speed, std::move(uuid), effects, type,
		          skipDelay);
	}
}

void TankSpawner::SpawnCoopBot(ObjRectangle rect, const float speed, const int health, const buuid uuid,
                               const TankType type, const bool skipDelay)
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
		std::string name{(type == TankType::COOP1 ? "CoopBot1" : "CoopBot2")};
		std::string fraction{"PlayerTeam"};

		// Log coop bot spawn
		const std::string uuidString = UuidUtils::GetStringUuid(uuid);
		Logger::GetInstance().LogTankSpawn(name, fraction, uuidString);
		std::cout << "[" << GetCurrentTimeString() << "] "
				<< "[" << (_gameMode == GameMode::PlayAsHost ? "SERVER" : "CLIENT") << "] "
				<< "SpawnEnemy  UUID = " << uuidString
				<< ", Name = " << name
				<< std::endl;

		constexpr int yellow{0xeaea00};
		constexpr int green{0x408000};
		const int color = type == TankType::COOP1 ? yellow : green;
		const bool isTimerActive = _bonusEffectManager->GetTimerPlayer().isActive;
		const bool isHelmetActive = _bonusEffectManager->GetHelmet(type == TankType::COOP1 ? 4 : 5).isActive;
		const BonusEffectProperty effects = {isTimerActive, isHelmetActive};

		SpawnTank(rect, color, health, std::move(name), std::move(fraction), speed, std::move(uuid), effects, type,
		          skipDelay);
	}
}

void TankSpawner::RespawnEnemyTanks(const TankType type, const buuid uuid, const bool skipDelay)
{
	constexpr float speed{142};
	constexpr int health{100};
	SpawnEnemy(uuid, type, speed, health, skipDelay);

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent("ServerSend_RespawnTank", type, uuid);
	}
}

void TankSpawner::RespawnPlayerTeam(const TankType type, const buuid uuid, const bool skipDelay)
{
	const float windowSizeY{static_cast<float>(_windowSize.y)};
	const float gridOffset{windowSizeY / 50.f};
	const float size{gridOffset * 3};
	constexpr float speed{142};
	constexpr int health{100};
	const bool isFirst = type == TankType::PLAYER1;
	const ObjRectangle rect{.x = gridOffset * (isFirst ? 16.f : 32.f), .y = windowSizeY - size, .w = size, .h = size};

	if (_gameMode == GameMode::OnePlayer
	    || _gameMode == GameMode::TwoPlayers
	    || _gameMode == GameMode::PlayAsHost
	    || _gameMode == GameMode::PlayAsClient
	    || _gameMode == GameMode::CoopWithBot && isFirst)
	{
		SpawnPlayer(rect, speed, health, uuid, type, skipDelay);
	}
	else if (_gameMode == GameMode::Demo || _gameMode == GameMode::CoopWithBot)
	{
		SpawnCoopBot(rect, speed, health, uuid, isFirst ? TankType::COOP1 : TankType::COOP2, skipDelay);
	}

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent("ServerSend_RespawnTank", type, uuid);
	}
}

void TankSpawner::RespawnTanks(const bool skipDelay)
{
	for (int i = 0; i < _slots.size(); ++i)
	{
		if (_slots[i].isAvailable)
		{
			switch (const auto type = static_cast<TankType>(i))
			{
				case TankType::ENEMY1:
				case TankType::ENEMY2:
				case TankType::ENEMY3:
				case TankType::ENEMY4:
					RespawnEnemyTanks(type, _slots[i].uuid, skipDelay);
					break;
				case TankType::PLAYER1:
				case TankType::PLAYER2:
					RespawnPlayerTeam(type, _slots[i].uuid, skipDelay);
					break;
				default:
					break;
			}
			_slots[i].isAvailable = false;
			break;
		}
	}
}

int TankSpawner::GetEnemyRespawnResource() const
{
	return _respawnResource[static_cast<std::size_t>(RespawnResource::ENEMY_ALL)];
}

int TankSpawner::GetPlayerOneRespawnResource() const
{
	return _respawnResource[static_cast<std::size_t>(RespawnResource::PLAYER_ONE)];
}

int TankSpawner::GetPlayerTwoRespawnResource() const
{
	return _respawnResource[static_cast<std::size_t>(RespawnResource::PLAYER_TWO)];
}

void TankSpawner::OnClientRespawn(const TankType type, const buuid uuid, const bool skipDelay)
{
	switch (type)
	{
		case TankType::ENEMY1:
		case TankType::ENEMY2:
		case TankType::ENEMY3:
		case TankType::ENEMY4:
			RespawnEnemyTanks(type, uuid, skipDelay);
			DecreaseEnemyRespawnResource();
			break;
		case TankType::PLAYER1:
			RespawnPlayerTeam(type, uuid, skipDelay);
			DecreasePlayerOneRespawnResource();
			break;
		case TankType::PLAYER2:
			RespawnPlayerTeam(type, uuid, skipDelay);
			DecreasePlayerTwoRespawnResource();
			break;
		default:
			break;
	}
}

void TankSpawner::IncreaseEnemyRespawnResource()
{
	constexpr auto id = static_cast<size_t>(RespawnResource::ENEMY_ALL);
	++_respawnResource[id];
	_events->EmitEvent("RespawnResourceChangedTo", "Enemy", _respawnResource[id]);
}

void TankSpawner::IncreasePlayerOneRespawnResource()//TODO: combine increase for both players into one
{
	constexpr auto id = static_cast<size_t>(RespawnResource::PLAYER_ONE);
	++_respawnResource[id];
	_events->EmitEvent("RespawnResourceChangedTo", "Player1", _respawnResource[id]);
}

void TankSpawner::IncreasePlayerTwoRespawnResource()
{
	constexpr auto id = static_cast<size_t>(RespawnResource::PLAYER_TWO);
	++_respawnResource[id];
	_events->EmitEvent("RespawnResourceChangedTo", "Player2", _respawnResource[id]);
}

void TankSpawner::DecreaseEnemyRespawnResource()
{
	constexpr auto id = static_cast<size_t>(RespawnResource::ENEMY_ALL);
	--_respawnResource[id];
	_events->EmitEvent("RespawnResourceChangedTo", "Enemy", _respawnResource[id]);
}

void TankSpawner::DecreasePlayerOneRespawnResource()
{
	constexpr auto id = static_cast<size_t>(RespawnResource::PLAYER_ONE);
	--_respawnResource[id];
	_events->EmitEvent("RespawnResourceChangedTo", "Player1", _respawnResource[id]);
}

void TankSpawner::DecreasePlayerTwoRespawnResource()
{
	constexpr auto id = static_cast<size_t>(RespawnResource::PLAYER_TWO);
	--_respawnResource[id];
	_events->EmitEvent("RespawnResourceChangedTo", "Player2", _respawnResource[id]);
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

	if (_gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent("ServerSend_OnTank", author, fraction);
	}
}

void TankSpawner::OnTankSpawn(const buuid& uuid)
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

std::unique_ptr<IInputProvider> TankSpawner::GetInputProvider(const TankType type)
{
	if (type == TankType::PLAYER1)
	{
		if (_gameMode == GameMode::PlayAsClient)
		{
			return std::make_unique<InputProviderForPlayerOneNet>(_events);
		}

		return std::make_unique<InputProviderForPlayerOne>(_events);
	}

	if (_gameMode == GameMode::PlayAsClient || _gameMode == GameMode::PlayAsHost)
	{
		return std::make_unique<InputProviderForPlayerTwoNet>(_events);
	}

	return std::make_unique<InputProviderForPlayerTwo>(_events);
}

std::shared_ptr<Tank> TankSpawner::CreateTank(const TankType type, PawnProperty pawnProperty,
                                              BonusEffectProperty effects)
{
	constexpr bool enableByDefault = true;

	if (type == TankType::ENEMY1 || type == TankType::ENEMY2 || type == TankType::ENEMY3 || type == TankType::ENEMY4)
	{
		return std::make_shared<Enemy>(std::move(pawnProperty), _bulletPool, std::move(effects), enableByDefault);
	}

	if (type == TankType::COOP1 || type == TankType::COOP2)
	{
		return std::make_shared<CoopBot>(std::move(pawnProperty), _bulletPool, std::move(effects), enableByDefault);
	}

	return std::make_shared<Player>(
			std::move(pawnProperty), _bulletPool, GetInputProvider(type), std::move(effects), enableByDefault);
}

void TankSpawner::SpawnTank(const ObjRectangle rect, const int color, const int health, const std::string& name,
                            const std::string& fraction, const float speed, buuid uuid, BonusEffectProperty effects,
                            const TankType type, const bool skipDelay)
{
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.color = color,
			.health = health,
			.uuid = uuid,
			.name = name,//TODO:move
			.fraction = std::move(fraction)};//TODO:move

	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = _allObjects,
			.events = _events,
			.tier = 1,
			.speed = speed,
			.windowSize = _windowSize,
			.dir = Direction::UP,
			.gameMode = _gameMode};

	if (std::shared_ptr<Tank> tank{CreateTank(type, std::move(pawnProperty), std::move(effects))})
	{
		_allObjects->emplace_back(tank);
		_events->EmitEvent("SpawnDelayStart", tank, milliseconds(skipDelay ? 0 : 1000));
		_events->EmitEvent("AnimationCreate", AnimationType::Spawn_Animation, rect, name, color);
	}
}
