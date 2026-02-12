#include "components/TankSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/input/InputProviderForPlayerOne.h"
#include "components/input/InputProviderForPlayerOneNet.h"
#include "components/input/InputProviderForPlayerTwo.h"
#include "components/input/InputProviderForPlayerTwoNet.h"
#include "components/managers/BonusEffectManager.h"
#include "components/managers/RespawnManager.h"
#include "entities/pawns/CoopBot.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/Player.h"
#include "enums/AnimationType.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
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
                         const std::shared_ptr<EventSystem>& events)
	: _windowSize{windowSize},
	  _allObjects{allObjects},
	  _events{events},
	  _bulletPool{std::make_shared<BulletPool>(events, allObjects, windowSize, GameMode::Demo)},
	  _bonusEffectManager{std::make_shared<BonusEffectManager>(events)},
	  _respawnManager{std::make_shared<RespawnManager>(events)}
//TODO: extract tank spawner to respawn manager as sub component
{
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
	_events->AddListener("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		this->_gameMode = newGameMode;

		this->_gameMode == GameMode::PlayAsClient ? SubscribeAsClient() : UnsubscribeAsClient();
	});

	_events->AddListener("SpawnEnabled", _name, [this](std::weak_ptr<Tank> tank)
	{
		std::shared_ptr<Tank> tankLck = tank.lock();
		if (!tankLck)
			return;//TODO: add assert for this case

		tankLck->Enable();
		_events->EmitEvent("AnimationCreateTank", tank);
	});

	_events->AddListener("PreTickUpdate", _name, [this](const double /*deltaTime*/) { this->RespawnTanks(); });
}

void TankSpawner::SubscribeAsClient()
{
	_events->AddListener("ClientReceived_RespawnTank", _name, [this](const TankType type, const buuid& uuid)
	{
		//TODO: if true we see tanks on client until they first die, refactor whole spawn flow on client
		constexpr bool skipDelay = true;
		this->OnClientRespawn(type, uuid, skipDelay);
	});
}

void TankSpawner::Unsubscribe() const
{
	_events->RemoveListener("GameModeChangedTo", _name);

	if (_gameMode == GameMode::PlayAsClient)
	{
		UnsubscribeAsClient();
	}

	_events->RemoveListener("SpawnEnabled", _name);
	_events->RemoveListener("PreTickUpdate", _name);
}

void TankSpawner::UnsubscribeAsClient() const
{
	_events->RemoveListener("ClientReceived_RespawnTank", _name);
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
			return ColliderUtils::IsCollide(rect, object->GetRect());
		});

		if (isFreeSpawnSpot)
		{
			const std::string name{"Enemy" + std::to_string(static_cast<int>(type) + 1)};
			std::string fraction{"EnemyTeam"};

			// Log enemy tank spawn
			const std::string uuidString = UuidUtils::GetStringUuid(uuid);
			Logger::GetInstance().LogTankSpawn(name, fraction, uuidString);
			std::cout << "[" << GetCurrentTimeString() << "] "
					<< "[" << (_gameMode == GameMode::PlayAsHost ? "SERVER" : "CLIENT") << "] "
					<< "SpawnEnemy  UUID = " << uuidString
					<< ", Name = " << name
					<< '\n';

			constexpr int gray{0x808080};
			const BonusEffectProperty effects = {
					.isTimerActive = _bonusEffectManager->GetTimerEnemy().isActive,
					.isHelmetActive = _bonusEffectManager->GetHelmet(static_cast<int>(type)).isActive
			};

			SpawnTank(rect, gray, health, name, std::move(fraction), speed, uuid, effects, type, skipDelay);
			_events->EmitEvent("EnemySpawned");
			return;
		}
	}
}

void TankSpawner::SpawnPlayer(ObjRectangle rect, const float speed, const int health, const buuid uuid,
                              const TankType type, const bool skipDelay)
{
	const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [&rect](const std::shared_ptr<BaseObj>& object)
	{
		return ColliderUtils::IsCollide(rect, object->GetRect());
	});

	if (isFreeSpawnSpot)
	{
		const bool isFirst = type == TankType::PLAYER1;
		const std::string name{isFirst ? "Player1" : "Player2"};
		std::string fraction{"PlayerTeam"};

		// Log tank spawn
		const std::string uuidString = UuidUtils::GetStringUuid(uuid);
		Logger::GetInstance().LogTankSpawn(name, fraction, uuidString);
		std::cout << "[" << GetCurrentTimeString() << "] "
				<< "[" << (_gameMode == GameMode::PlayAsHost ? "SERVER" : "CLIENT") << "] "
				<< "SpawnPlayer UUID = " << uuidString
				<< ", Name = " << name
				<< '\n';

		constexpr int yellow{0xeaea00};
		constexpr int green{0x408000};
		const unsigned int color = isFirst ? yellow : green;
		const BonusEffectProperty effects = {
				.isTimerActive = _bonusEffectManager->GetTimerPlayer().isActive,
				.isHelmetActive = _bonusEffectManager->GetHelmet(isFirst ? 4 : 5).isActive
				// Set as "true" to activate invincibility
		};

		SpawnTank(rect, color, health, name, std::move(fraction), speed, uuid, effects, type, skipDelay);
	}
}

void TankSpawner::SpawnCoopBot(ObjRectangle rect, const float speed, const int health, const buuid uuid,
                               const TankType type, const bool skipDelay)
{
	const bool isFreeSpawnSpot = !std::ranges::any_of(*_allObjects, [&rect](const std::shared_ptr<BaseObj>& object)
	{
		return ColliderUtils::IsCollide(rect, object->GetRect());
	});

	if (isFreeSpawnSpot)
	{
		const std::string name{(type == TankType::COOP1 ? "CoopBot1" : "CoopBot2")};
		std::string fraction{"PlayerTeam"};

		// Log coop bot spawn
		const std::string uuidString = UuidUtils::GetStringUuid(uuid);
		Logger::GetInstance().LogTankSpawn(name, fraction, uuidString);
		std::cout << "[" << GetCurrentTimeString() << "] "
				<< "[" << (_gameMode == GameMode::PlayAsHost ? "SERVER" : "CLIENT") << "] "
				<< "SpawnEnemy  UUID = " << uuidString
				<< ", Name = " << name
				<< '\n';

		constexpr int yellow{0xeaea00};
		constexpr int green{0x408000};
		const unsigned int color = type == TankType::COOP1 ? yellow : green;
		const BonusEffectProperty effects = {
				.isTimerActive = _bonusEffectManager->GetTimerPlayer().isActive,
				.isHelmetActive = _bonusEffectManager->GetHelmet(type == TankType::COOP1 ? 4 : 5).isActive
		};

		SpawnTank(rect, color, health, name, std::move(fraction), speed, uuid, effects, type, skipDelay);
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
	for (size_t i = 0; i < _respawnManager->_slots.size(); ++i)
	{
		if (_respawnManager->_slots[i].isAvailable)
		{
			switch (const auto type = static_cast<TankType>(i))
			{
				case TankType::ENEMY1:
				case TankType::ENEMY2:
				case TankType::ENEMY3:
				case TankType::ENEMY4:
					RespawnEnemyTanks(type, _respawnManager->_slots[i].uuid, skipDelay);
					break;
				case TankType::PLAYER1:
				case TankType::PLAYER2:
					RespawnPlayerTeam(type, _respawnManager->_slots[i].uuid, skipDelay);
					break;
				default:
					break;
			}
			_respawnManager->_slots[i].isAvailable = false;
			break;
		}
	}
}

int TankSpawner::GetEnemyRespawnCount() const { return _respawnManager->GetEnemyRespawnCount(); }

int TankSpawner::GetPlayerOneRespawnCount() const { return _respawnManager->GetPlayerOneRespawnCount(); }

int TankSpawner::GetPlayerTwoRespawnCount() const { return _respawnManager->GetPlayerTwoRespawnCount(); }

void TankSpawner::OnClientRespawn(const TankType type, const buuid uuid, const bool skipDelay)
{
	switch (type)
	{
		case TankType::ENEMY1:
		case TankType::ENEMY2:
		case TankType::ENEMY3:
		case TankType::ENEMY4:
			RespawnEnemyTanks(type, uuid, skipDelay);
			break;
		case TankType::PLAYER1:
		case TankType::PLAYER2:
			RespawnPlayerTeam(type, uuid, skipDelay);
			break;
		default:
			break;
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
	if (type == TankType::ENEMY1 || type == TankType::ENEMY2 || type == TankType::ENEMY3 || type == TankType::ENEMY4)
	{
		return std::make_shared<Enemy>(std::move(pawnProperty), _bulletPool, effects);
	}

	if (type == TankType::COOP1 || type == TankType::COOP2)
	{
		return std::make_shared<CoopBot>(std::move(pawnProperty), _bulletPool, effects);
	}

	return std::make_shared<Player>(std::move(pawnProperty), _bulletPool, GetInputProvider(type), effects);
}

void TankSpawner::SpawnTank(const ObjRectangle rect, const unsigned int color, const int health,
                            const std::string& name, std::string fraction, const float speed, buuid uuid,
                            BonusEffectProperty effects, const TankType type, const bool skipDelay)
{
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.color = color,
			.health = health,
			.uuid = uuid,
			.name = name,
			.fraction = std::move(fraction)};

	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = _allObjects,
			.events = _events,
			.tier = 1,
			.speed = speed,
			.windowSize = _windowSize,
			.dir = Direction::UP,
			.gameMode = _gameMode};

	if (std::shared_ptr<Tank> tank{CreateTank(type, std::move(pawnProperty), effects)})
	{
		_allObjects->emplace_back(tank);
		_events->EmitEvent("SpawnDelayStart", tank, milliseconds(skipDelay ? 0 : 1000));
		_events->EmitEvent("AnimationCreate", AnimationType::Spawn_Animation, rect, name);
	}
}
