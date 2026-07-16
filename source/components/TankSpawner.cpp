#include "components/TankSpawner.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/input/InputProviderForPlayerOne.h"
#include "components/input/InputProviderForPlayerOneNet.h"
#include "components/input/InputProviderForPlayerTwo.h"
#include "components/input/InputProviderForPlayerTwoNet.h"
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
#include "utils/RandUtils.h"
#include "utils/TimeUtils.h"
#include "utils/UuidUtils.h"
#include <algorithm>
#include <boost/uuid/uuid.hpp>
#include <iostream>
#include <memory>

TankSpawner::TankSpawner(GameConfig& gameConfig, std::vector<std::shared_ptr<BaseObj>>* allObjects,
						 const std::shared_ptr<EventSystem>& events)
	: _allObjects{allObjects}
	, _events{events}
	, _bulletPool{std::make_shared<BulletPool>(events, allObjects, gameConfig.windowSize, GameMode::Demo)}
	, _respawnManager{std::make_shared<RespawnManager>(events)}
	, _gameConfig{gameConfig}
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
	_events->AddListener("Reset", _name, [this]() { this->Reset(); });

	//TODO: reuse existing tanks when game mode changed
	_events->AddListener(
			"GameModeChangedTo", _name,
			[this](const GameMode newGameMode)
			{
				this->_gameMode = newGameMode;

				this->_gameMode == GameMode::PlayAsClient ? SubscribeAsClient() : UnsubscribeAsClient();
			});

	_events->AddListener(
			"SpawnEnabled", _name,
			[this](std::shared_ptr<Tank> tank)
			{
				if (!tank)
				{
					return;
				}

				tank->Enable();
				const ObjRectangle rect{tank->GetRect()};
				const std::string name{tank->GetName()};
				_events->EmitEvent("AnimationCreateTank", rect, name);
			});

	_events->AddListener("RespawnTanks", _name, [this](const double /*deltaTime*/)
	{
		this->RespawnTanks();
	});

	_events->AddListener("WindowSizeChangedTo", _name, [this](const UPoint& newSize)
	{
		_gameConfig.previousScaleFactor = _gameConfig.scaleFactor;
		_gameConfig.scaleFactor = static_cast<float>(newSize.y) / static_cast<float>(_gameConfig.windowSizeDefault.y);
		_gameConfig.gridSize = _gameConfig.scaleFactor;

		_gameConfig.gridOffset = _gameConfig.gridSizeDefault / _gameConfig.gridSize;
		_gameConfig.tankSize = _gameConfig.gridOffset * 3.f;

		_gameConfig.tankSpeed = _gameConfig.tankSpeed * _gameConfig.scaleFactor / 2.f;

		_gameConfig.bonusSize = static_cast<int>(_gameConfig.gridOffset * 3.f);

		//TODO:scale health bar

		//scale bullet calibre
		_events->EmitEvent("ScaleFactorChangedTo", _gameConfig.scaleFactor);
	});
}

void TankSpawner::SubscribeAsClient()
{
	_events->AddListener(
			"ClientReceived_RespawnTank", _name,
			[this](const TankType type, const buuid& uuid)
			{
				this->OnClientRespawn(type, uuid);
			});
}

void TankSpawner::Unsubscribe() const { _events->RemoveAllListeners(_name); }

void TankSpawner::UnsubscribeAsClient() const { _events->RemoveListener("ClientReceived_RespawnTank", _name); }

void TankSpawner::Reset()
{
	_enemySpawnTimer.cooldown = milliseconds{5000};
	_enemySpawnTimer.isActive = false;
	_enemySpawnTimer.activateTime = std::chrono::system_clock::now() - _enemySpawnTimer.cooldown;
}

std::string TankSpawner::GetCurrentTimeString()
{
	const auto now = std::chrono::system_clock::now();
	const auto nowTime = std::chrono::system_clock::to_time_t(now);
	const auto ms = std::chrono::duration_cast<milliseconds>(now.time_since_epoch()) % 1000;

	std::tm timeInfo;
	std::ignore = localtime_s(&timeInfo, &nowTime);

	std::stringstream ss;
	ss << std::put_time(&timeInfo, "%H:%M:%S") << '.'
			<< std::setfill('0') << std::setw(3) << ms.count();

	return ss.str();
}

ObjRectangle TankSpawner::GetEnemyRandomPosX(const TankType type) const
{
	const float tankSize{_gameConfig.tankSize};
	ObjRectangle rect{.x = -1.f, .y = -1.f, .w = tankSize, .h = tankSize};
	if (type > TankType::ENEMY4)
	{
		return rect;
	}

	// const float gridOffset{_gameConfig.gridOffset};
	const float battleFieldSizeX{static_cast<float>(_gameConfig.windowSize.x - _gameConfig.sideBarWidth)};

	const int quartFieldSizeX = static_cast<int>(battleFieldSizeX / 4.f - tankSize);
	const std::vector<std::pair<float, float>> spawnRanges{{0, quartFieldSizeX},
														   {quartFieldSizeX, quartFieldSizeX * 2},
														   {quartFieldSizeX * 2, quartFieldSizeX * 3},
														   {quartFieldSizeX * 3, quartFieldSizeX * 4}};


	const int randomRange = static_cast<int>(type);
	auto [minX, maxX] = spawnRanges[randomRange];
	//TODO: refactor uniform to float
	const std::uniform_int_distribution<> distRandX{static_cast<int>(minX), static_cast<int>(maxX)};
	const int randomX = RandUtils::GetRandNumber(distRandX);

	ObjRectangle spawnPos{.x = static_cast<float>(randomX), .y = 0, .w = tankSize, .h = tankSize};
	auto isCollidePredicate = [&spawnPos](const auto& object)
	{
		return ColliderUtils::IsCollide(spawnPos, object->GetRect());
	};

	if (!std::ranges::any_of(*_allObjects, isCollidePredicate))
	{
		rect = spawnPos;
	}
	else
	{
		float spawnX = minX;
		while (spawnX < maxX)
		{
			spawnPos.x = spawnX;
			if (!std::ranges::any_of(*_allObjects, isCollidePredicate))
			{
				rect = spawnPos;
				break;
			}
			spawnX += tankSize / 2.f;
		}
	}

	return rect;
}

bool TankSpawner::SpawnEnemy(const buuid uuid, const TankType type, const float speed, const int health,
							 const bool skipDelay)
{
	const ObjRectangle rect = GetEnemyRandomPosX(type);
	if (rect.y == -1.f)
	{
		return false;
	}

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

	SpawnTank(rect, health, name, std::move(fraction), speed, uuid, type, skipDelay);

	return true;
}

void TankSpawner::SpawnPlayer(const ObjRectangle rect, const float speed, const int health, const buuid uuid,
							  const TankType type, const bool skipDelay)
{
	const bool isFirst = type == TankType::PLAYER1;
	const std::string name{isFirst ? "Player1" : "Player2"};
	std::string fraction{"PlayerTeam"};

	// Log tank spawn
	const std::string uuidString = UuidUtils::GetStringUuid(uuid);
	Logger::GetInstance().LogTankSpawn(name, fraction, uuidString);
	std::cout << "[" << GetCurrentTimeString() << "] "
			<< "[" << (_gameMode == GameMode::PlayAsHost ? "SERVER" : "CLIENT") << "] "
			<< "SpawnPlayer UUID = " << uuidString << ", Name = " << name << '\n';

	SpawnTank(rect, health, name, std::move(fraction), speed, uuid, type, skipDelay);
}

void TankSpawner::SpawnCoopBot(const ObjRectangle rect, const float speed, const int health, const buuid uuid,
							   const TankType type, const bool skipDelay)
{
	const std::string name{(type == TankType::COOP1 ? "CoopBot1" : "CoopBot2")};
	std::string fraction{"PlayerTeam"};

	// Log coop bot spawn
	const std::string uuidString = UuidUtils::GetStringUuid(uuid);
	Logger::GetInstance().LogTankSpawn(name, fraction, uuidString);
	std::cout << "[" << GetCurrentTimeString() << "] "
			<< "[" << (_gameMode == GameMode::PlayAsHost ? "SERVER" : "CLIENT") << "] "
			<< "SpawnEnemy  UUID = " << uuidString << ", Name = " << name << '\n';

	SpawnTank(rect, health, name, std::move(fraction), speed, uuid, type, skipDelay);
}

void TankSpawner::RespawnEnemyTanks(const TankType type, const buuid uuid, const bool skipDelay)
{
	const bool isSuccessSpawn = SpawnEnemy(uuid, type, _gameConfig.tankSpeed, _gameConfig.tankHealth, skipDelay);
	if (isSuccessSpawn && _gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent("ServerSend_RespawnTank", type, uuid);
	}
}

ObjRectangle TankSpawner::GetPlayerRandomPosX(const bool isFirst) const
{
	const float windowSizeX{static_cast<float>(_gameConfig.windowSize.x - _gameConfig.sideBarWidth)};
	const float windowSizeY{static_cast<float>(_gameConfig.windowSize.y)};
	// const float gridOffset{_gameConfig.gridOffset};
	const float tankSize{_gameConfig.tankSize};

	const std::pair<float, float> spawnRangePlayer1{0,
													static_cast<int>(windowSizeX / 2.f - tankSize * 3.25f)};
	const std::pair<float, float> spawnRangePlayer2{static_cast<int>(windowSizeX / 2.f + tankSize * 2.25f),
													static_cast<int>(windowSizeX)};
	auto [minX, maxX]{isFirst ? spawnRangePlayer1 : spawnRangePlayer2};

	const std::uniform_int_distribution<> distRandId{static_cast<int>(minX), static_cast<int>(maxX)};
	const int randomX = RandUtils::GetRandNumber(distRandId);

	ObjRectangle rect{.x = -1.f, .y = -1.f, .w = tankSize, .h = tankSize};
	ObjRectangle spawnPos{.x = static_cast<float>(randomX), .y = windowSizeY - tankSize, .w = tankSize, .h = tankSize};
	auto isCollidePredicate = [&spawnPos](const auto& object)
	{
		return ColliderUtils::IsCollide(spawnPos, object->GetRect());
	};

	if (!std::ranges::any_of(*_allObjects, isCollidePredicate))
	{
		rect = spawnPos;
	}
	else
	{
		float spawnX = minX;
		while (spawnX < maxX)
		{
			spawnPos.x = spawnX;
			if (!std::ranges::any_of(*_allObjects, isCollidePredicate))
			{
				rect = spawnPos;
				break;
			}

			spawnX += tankSize / 2.f;
		}
	}

	return rect;
}

void TankSpawner::RespawnPlayerTeam(const TankType type, const buuid uuid, const bool skipDelay)
{
	const bool isFirst = type == TankType::PLAYER1;
	const ObjRectangle rect{GetPlayerRandomPosX(isFirst)};
	if (rect.y == -1.f)
	{
		return;
	}

	if (_gameMode == GameMode::OnePlayer
		|| _gameMode == GameMode::TwoPlayers
		|| _gameMode == GameMode::PlayAsHost
		|| _gameMode == GameMode::PlayAsClient
		|| _gameMode == GameMode::CoopWithBot && isFirst)
	{
		SpawnPlayer(rect, _gameConfig.tankSpeed, _gameConfig.tankHealth, uuid, type, skipDelay);
		if (_gameMode == GameMode::PlayAsHost)
		{
			_events->EmitEvent("ServerSend_RespawnTank", type, uuid);
		}
	}
	else if (_gameMode == GameMode::Demo || _gameMode == GameMode::CoopWithBot)
	{
		SpawnCoopBot(rect, _gameConfig.tankSpeed, _gameConfig.tankHealth, uuid,
					 isFirst ? TankType::COOP1 : TankType::COOP2, skipDelay);
	}
}

void TankSpawner::RespawnTank(const TankType type, const buuid uuid, const bool skipDelay)
{
	switch (type)
	{
		case TankType::ENEMY1:
		case TankType::ENEMY2:
		case TankType::ENEMY3:
		case TankType::ENEMY4:
			if (skipDelay)
			{
				RespawnEnemyTanks(type, uuid, skipDelay);
			}
			else if (TimeUtils::IsCooldownFinish(_enemySpawnTimer.activateTime, _enemySpawnTimer.cooldown))
			{
				_enemySpawnTimer.Reset();
				RespawnEnemyTanks(type, uuid, skipDelay);
			}
			break;
		case TankType::PLAYER1:
		case TankType::PLAYER2:
		case TankType::COOP1:
		case TankType::COOP2:
			RespawnPlayerTeam(type, uuid, skipDelay);
			break;
	}
}

void TankSpawner::RespawnTanks(const bool skipDelay)
{
	for (size_t i = 0; i < _respawnManager->_slots.size(); ++i)
	{
		if (const auto [uuid, isAvailable] = _respawnManager->_slots[i];
			isAvailable)
		{
			RespawnTank(static_cast<TankType>(i), uuid, skipDelay);
		}
	}
}

int TankSpawner::GetEnemyRespawnCount() const { return _respawnManager->GetEnemyRespawnCount(); }

int TankSpawner::GetPlayerOneRespawnCount() const { return _respawnManager->GetPlayerOneRespawnCount(); }

int TankSpawner::GetPlayerTwoRespawnCount() const { return _respawnManager->GetPlayerTwoRespawnCount(); }

void TankSpawner::OnClientRespawn(const TankType type, const buuid uuid)
{
	constexpr bool skipDelay{false};
	RespawnTank(type, uuid, skipDelay);
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

std::shared_ptr<Tank> TankSpawner::CreateTank(const TankType type, PawnProperty pawnProperty)
{
	if (type == TankType::ENEMY1 || type == TankType::ENEMY2 || type == TankType::ENEMY3 || type == TankType::ENEMY4)
	{
		return std::make_shared<Enemy>(std::move(pawnProperty), _bulletPool, _gameConfig);
	}

	if (type == TankType::COOP1 || type == TankType::COOP2)
	{
		return std::make_shared<CoopBot>(std::move(pawnProperty), _bulletPool, _gameConfig);
	}

	return std::make_shared<Player>(std::move(pawnProperty), _bulletPool, GetInputProvider(type), _gameConfig);
}

void TankSpawner::SpawnTank(const ObjRectangle rect, const int health, const std::string& name, std::string fraction,
							const float speed, buuid uuid, const TankType type, const bool skipDelay)
{
	BaseObjProperty baseObjProperty{.rect = rect,
									.health = health,
									.uuid = uuid,
									.name = name,
									.fraction = std::move(fraction)};

	PawnProperty pawnProperty{.baseObjProperty = std::move(baseObjProperty),
							  .allObjects = _allObjects,
							  .events = _events,
							  .tier = 1u,
							  .speed = speed,
							  .windowSize = _gameConfig.windowSize,
							  .dir = Direction::UP,
							  .gameMode = _gameMode};

	if (std::shared_ptr<Tank> tank{CreateTank(type, std::move(pawnProperty))})
	{
		_allObjects->emplace_back(tank);
		_events->EmitEvent("SpawnDelayStart", tank, milliseconds(skipDelay ? 0 : 1000));

		if (_gameMode != GameMode::PlayAsClient)
		{
			_events->EmitEvent("AnimationCreate", AnimationType::Spawn_Animation, rect, name);
		}
	}
}
