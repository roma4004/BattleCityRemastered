#include "components/TankSpawner.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/input/InputProviderForPlayerOne.h"
#include "components/input/InputProviderForPlayerOneNet.h"
#include "components/input/InputProviderForPlayerTwo.h"
#include "components/input/InputProviderForPlayerTwoNet.h"
#include "components/events/SpawnEvents.h"
#include "components/events/AnimationRenderEvents.h"
#include "entities/pawns/CoopBot.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/Player.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/TankType.h"
#include "utils/ColliderUtils.h"
#include "utils/Logger.h"
#include "utils/RandUtils.h"
#include "utils/TimeUtils.h"
#include "utils/Uuid.h"
#include "utils/UuidUtils.h"
#include <algorithm>
#include <iomanip>
#include <iostream>
#include <memory>

TankSpawner::TankSpawner(GameConfig& gameConfig, std::vector<std::shared_ptr<BaseObj>>* allObjects,
						 const std::shared_ptr<EventSystem>& events)
	: _allObjects{allObjects}
	, _events{events}
	, _bulletPool{std::make_shared<BulletPool>(events, allObjects, gameConfig)}
	, _gameConfig{gameConfig}
{
	Subscribe();
}

void TankSpawner::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &TankSpawner::Reset));

	//TODO: reuse existing tanks when game mode changed
	_subs.push_back(_events->AddListener(this, &TankSpawner::OnGameModeChangedTo));
	_subs.push_back(_events->AddListener(this, &TankSpawner::OnRespawnTank));
	_subs.push_back(_events->AddListener(this, &TankSpawner::OnTankSpawnDelayFinished));
}

void TankSpawner::OnGameModeChangedTo(const GameModeChangedToEvent& event)
{
	_gameMode = event.mode;

	_gameMode == GameMode::PlayAsClient ? SubscribeAsClient() : UnsubscribeAsClient();
}

void TankSpawner::OnRespawnTank(const RespawnTankEvent& event) { RespawnTank(event.type, event.uuid, event.skipDelay); }

void TankSpawner::OnTankSpawnDelayFinished(const TankSpawnDelayFinishedEvent& event)
{
	OnSpawnDelayFinished(event.uuid);
}

void TankSpawner::SubscribeAsClient()
{
	_clientRespawnSub = _events->AddListener(this, &TankSpawner::OnClientInRespawnTank);

	// Sole materialize trigger on the client - DelayedSpawnManager's timer doesn't tick here.
	_clientMaterializeSub = _events->AddListener(this, &TankSpawner::OnClientInTankSpawnComplete);
}

void TankSpawner::OnClientInRespawnTank(const ClientInRespawnTankEvent& event)
{
	OnClientRespawn(event.type, event.uuid, event.rect);
}

void TankSpawner::OnClientInTankSpawnComplete(const ClientInTankSpawnCompleteEvent& event)
{
	OnSpawnDelayFinished(event.uuid);
}

void TankSpawner::UnsubscribeAsClient()
{
	_clientRespawnSub = EventSubscription{};
	_clientMaterializeSub = EventSubscription{};
}

void TankSpawner::Reset(const GameResetEvent&)
{
	_enemySpawnTimer.cooldown = milliseconds{5000};
	_enemySpawnTimer.isActive = false;
	_enemySpawnTimer.activateTime = std::chrono::system_clock::now() - _enemySpawnTimer.cooldown;
	_delayedSpawns.clear();
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

	const float battleFieldSizeX{static_cast<float>(_gameConfig.windowSize.x - _gameConfig.sideBarWidth) - tankSize};

	const float quartFieldSizeX = battleFieldSizeX / 4.f;
	const std::vector<std::pair<float, float>> spawnRanges{{0.f, quartFieldSizeX},
														   {quartFieldSizeX, quartFieldSizeX * 2.f},
														   {quartFieldSizeX * 2.f, quartFieldSizeX * 3.f},
														   {quartFieldSizeX * 3.f, battleFieldSizeX}};


	const auto randomRange = static_cast<std::size_t>(type);
	auto [minX, maxX] = spawnRanges[randomRange];
	const std::uniform_real_distribution<float> distRandX{minX, maxX};
	const float randomX = RandUtils::GetRandNumber(distRandX);

	ObjRectangle spawnPos{.x = randomX, .y = 0, .w = tankSize, .h = tankSize};
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

bool TankSpawner::SpawnEnemy(const ObjRectangle rect, const Uuid uuid, const TankType type, const float speed,
							 const int health, const bool skipDelay)
{
	if (ColliderUtils::AreEqualAbsolute(rect.y, -1.f))
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

	DelayedSpawnStart(rect, health, name, std::move(fraction), speed, uuid, type, skipDelay);

	return true;
}

void TankSpawner::SpawnPlayer(const ObjRectangle rect, const float speed, const int health, const Uuid uuid,
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

	DelayedSpawnStart(rect, health, name, std::move(fraction), speed, uuid, type, skipDelay);
}

void TankSpawner::SpawnCoopBot(const ObjRectangle rect, const float speed, const int health, const Uuid uuid,
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

	DelayedSpawnStart(rect, health, name, std::move(fraction), speed, uuid, type, skipDelay);
}

void TankSpawner::RespawnEnemyTanks(const TankType type, const Uuid uuid, const bool skipDelay,
									const std::optional<ObjRectangle> rect)
{
	const ObjRectangle spawnRect = rect.has_value() ? *rect : GetEnemyRandomPosX(type);
	const bool isSuccessSpawn = SpawnEnemy(spawnRect, uuid, type, _gameConfig.tankSpeed, _gameConfig.tankHealth,
										   skipDelay);
	if (isSuccessSpawn && _gameMode == GameMode::PlayAsHost)
	{
		_events->EmitEvent(ServerOutRespawnTankEvent{.type = type, .uuid = uuid, .rect = spawnRect});
	}
}

//TODO: write unit test for bot change direction if faced obstacle
ObjRectangle TankSpawner::GetPlayerRandomPosX(const bool isFirst) const
{
	const float windowSizeX{static_cast<float>(_gameConfig.windowSize.x - _gameConfig.sideBarWidth)};
	const float windowSizeY{static_cast<float>(_gameConfig.windowSize.y)};
	// const float gridOffset{_gameConfig.gridOffset};
	const float tankSize{_gameConfig.tankSize};

	const std::pair spawnRangePlayer1{0.f, windowSizeX / 2.f - tankSize * 3.25f};
	const std::pair spawnRangePlayer2{windowSizeX / 2.f + tankSize * 2.25f, windowSizeX - tankSize};
	auto [minX, maxX]{isFirst ? spawnRangePlayer1 : spawnRangePlayer2};

	const std::uniform_real_distribution distRandId{minX, maxX};
	const float randomX = RandUtils::GetRandNumber(distRandId);

	ObjRectangle rect{.x = -1.f, .y = -1.f, .w = tankSize, .h = tankSize};
	ObjRectangle spawnPos{.x = randomX, .y = windowSizeY - tankSize, .w = tankSize, .h = tankSize};
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

void TankSpawner::RespawnPlayerTeam(const TankType type, const Uuid uuid, const bool skipDelay,
									const std::optional<ObjRectangle> rect)
{
	const bool isFirst = type == TankType::PLAYER1;
	const ObjRectangle spawnRect{rect.has_value() ? *rect : GetPlayerRandomPosX(isFirst)};
	if (ColliderUtils::AreEqualAbsolute(spawnRect.y, -1.f))
	{
		return;
	}

	if (_gameMode == GameMode::OnePlayer
		|| _gameMode == GameMode::TwoPlayers
		|| _gameMode == GameMode::PlayAsHost
		|| _gameMode == GameMode::PlayAsClient
		|| (_gameMode == GameMode::CoopWithBot && isFirst))
	{
		SpawnPlayer(spawnRect, _gameConfig.tankSpeed, _gameConfig.tankHealth, uuid, type, skipDelay);
		if (_gameMode == GameMode::PlayAsHost)
		{
			_events->EmitEvent(ServerOutRespawnTankEvent{.type = type, .uuid = uuid, .rect = spawnRect});
		}
	}
	else if (_gameMode == GameMode::Demo || _gameMode == GameMode::CoopWithBot)
	{
		SpawnCoopBot(spawnRect, _gameConfig.tankSpeed, _gameConfig.tankHealth, uuid,
					 isFirst ? TankType::COOP1 : TankType::COOP2, skipDelay);
	}
}

void TankSpawner::RespawnTank(const TankType type, const Uuid uuid, const bool skipDelay,
							  const std::optional<ObjRectangle> rect)
{
	switch (type)
	{
		case TankType::ENEMY1:
		case TankType::ENEMY2:
		case TankType::ENEMY3:
		case TankType::ENEMY4:
		{
			const bool isNetworkMirrored = rect.has_value();// server already decided when to spawn this tank
			if (skipDelay || isNetworkMirrored)
			{
				RespawnEnemyTanks(type, uuid, skipDelay, rect);
			}
			else if (TimeUtils::IsCooldownFinish(_enemySpawnTimer.activateTime, _enemySpawnTimer.cooldown))
			{
				_enemySpawnTimer.Reset();
				RespawnEnemyTanks(type, uuid, skipDelay, rect);
			}
			break;
		}
		case TankType::PLAYER1:
		case TankType::PLAYER2:
		case TankType::COOP1:
		case TankType::COOP2:
			RespawnPlayerTeam(type, uuid, skipDelay, rect);
			break;
	}
}

//TODO: maybe we don't need spawn on client at all and just move the textures and animation?
void TankSpawner::OnClientRespawn(const TankType type, const Uuid uuid, const ObjRectangle rect)
{
	constexpr bool skipDelay{false};
	RespawnTank(type, uuid, skipDelay, rect);
}

namespace
{
template<typename TLocal, typename TNet>
std::unique_ptr<IInputProvider> MakeProvider(const bool isNet, const std::shared_ptr<EventSystem>& events)
{
	if (isNet)
	{
		return std::make_unique<TNet>(events);
	}

	return std::make_unique<TLocal>(events);
}
}

std::unique_ptr<IInputProvider> TankSpawner::GetInputProvider(const TankType type) const
{
	const bool isFirst = type == TankType::PLAYER1;
	const bool isNet = _gameMode == GameMode::PlayAsClient
					   || (_gameMode == GameMode::PlayAsHost && !isFirst);

	return isFirst
			   ? MakeProvider<InputProviderForPlayerOne, InputProviderForPlayerOneNet>(isNet, _events)
			   : MakeProvider<InputProviderForPlayerTwo, InputProviderForPlayerTwoNet>(isNet, _events);
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

void TankSpawner::DelayedSpawnStart(const ObjRectangle rect, const int health, const std::string& name,
									std::string fraction, const float speed, const Uuid uuid, const TankType type,
									const bool skipDelay)
{
	_delayedSpawns.push_back(DelayedTankSpawn{.uuid = uuid,
											  .type = type,
											  .rect = rect,
											  .health = health,
											  .name = name,
											  .fraction = std::move(fraction),
											  .speed = speed});

	_events->EmitEvent(TankSpawnEvent{.uuid = uuid});

	// On a real client (skipDelay false) skip this: DelayedSpawnManager's timer never ticks there,
	// so a >0ms entry would just sit in _spawnDelays forever, never disposed.
	if (skipDelay || _gameMode != GameMode::PlayAsClient)
	{
		const milliseconds delay{skipDelay ? 0 : 1000};
		_events->EmitEvent(SpawnDelayStartEvent{.uuid = uuid, .delay = delay});
	}

	_events->EmitEvent(AnimationCreateTankSpawnEvent{.rect = rect, .name = name});
}

void TankSpawner::OnSpawnDelayFinished(const Uuid uuid)
{
	const auto it = std::ranges::find(_delayedSpawns, uuid, &DelayedTankSpawn::uuid);
	if (it == _delayedSpawns.end())
	{
		return;
	}

	DelayedSpawnWith(*it);
	_delayedSpawns.erase(it);
}

void TankSpawner::DelayedSpawnWith(const DelayedTankSpawn& params)
{
	BaseObjProperty baseObjProperty{.rect = params.rect,
									.health = params.health,
									.uuid = params.uuid,
									.name = params.name,
									.fraction = params.fraction};

	PawnProperty pawnProperty{.baseObjProperty = std::move(baseObjProperty),
							  .allObjects = _allObjects,
							  .events = _events,
							  .tier = 1u,
							  .speed = params.speed,
							  .dir = Direction::UP,
							  .gameMode = _gameMode};

	if (const std::shared_ptr<BaseObj> tank{CreateTank(params.type, std::move(pawnProperty))})
	{
		_events->EmitEvent(AddToSpawnQueueEvent{.obj = tank});
		_events->EmitEvent(AnimationCreateTankMoveEvent{.rect = params.rect, .name = params.name});
		_events->EmitEvent(BonusReApplyEvent{.uuid = params.uuid, .name = params.name, .fraction = params.fraction});

		if (_gameMode == GameMode::PlayAsHost)
		{
			_events->EmitEvent(ServerOutTankSpawnCompleteEvent{.uuid = params.uuid});
		}
	}
}
