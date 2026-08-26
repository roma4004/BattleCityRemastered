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
#include "enums/Faction.h"
#include "utils/Log.h"
#include "utils/RandUtils.h"
#include "utils/TimeUtils.h"
#include "utils/Uuid.h"
#include "utils/UuidUtils.h"
#include <algorithm>
#include <memory>

TankSpawner::TankSpawner(const GameConfig& gameConfig, std::vector<std::shared_ptr<BaseObj>>* allObjects,
						 const std::shared_ptr<EventSystem>& events)
	: _allObjects{allObjects}
	, _events{events}
	, _bulletPool{std::make_shared<BulletPool>(events, allObjects, gameConfig)}
	, _gameMode{gameConfig.gameMode}
	, _gameConfig{gameConfig}
{
	Subscribe();
}

void TankSpawner::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &TankSpawner::Reset));
	_subs.push_back(_events->AddListener(this, &TankSpawner::OnRespawnTank));
	//NOTE: a tank is the host's call - the client ignores its own burst and waits for TankSpawnComplete
	if (IsAuthority(_gameMode))
	{
		_subs.push_back(_events->AddListener(this, &TankSpawner::OnSpawnAnimationFinished));
	}

	if (IsClient(_gameMode))
	{
		_subs.push_back(_events->AddListener(this, &TankSpawner::OnTankRespawned));
		_subs.push_back(_events->AddListener(this, &TankSpawner::OnTankSpawnCompleted));
	}
}

void TankSpawner::OnRespawnTank(const RespawnTankEvent& event) { RespawnTank(event.type, event.uuid); }

void TankSpawner::OnSpawnAnimationFinished(const SpawnAnimationFinishedEvent& event)
{
	OnSpawnDelayFinished(event.uuid);
}

void TankSpawner::OnTankRespawned(const TankRespawnedEvent& event)
{
	const float tankSize{_gameConfig.tankSize};
	OnClientRespawn(event.type, event.uuid,
					ObjRectangle{.x = event.pos.x, .y = event.pos.y, .w = tankSize, .h = tankSize});
}

void TankSpawner::OnTankSpawnCompleted(const TankSpawnCompletedEvent& event)
{
	OnSpawnDelayFinished(event.uuid);
}

void TankSpawner::Reset(const GameResetEvent&)
{
	_enemySpawnTimer.cooldown = _gameConfig.enemySpawnCooldown;
	_enemySpawnTimer.isActive = false;
	_enemySpawnTimer.activateTime = TimeUtils::Now() - _enemySpawnTimer.cooldown;
	_delayedSpawns.clear();
}

ObjRectangle TankSpawner::GetEnemyRandomPosX(const TankType type) const
{
	const float tankSize{_gameConfig.tankSize};
	ObjRectangle rect{.x = -1.f, .y = -1.f, .w = tankSize, .h = tankSize};
	if (type > TankType::ENEMY4)
	{
		return rect;
	}

	const float battleFieldSizeX{static_cast<float>(_gameConfig.battlefieldSize.x) - tankSize};

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
							 const int health)
{
	if (ColliderUtils::AreEqualAbsolute(rect.y, -1.f))
	{
		return false;
	}

	const std::string name{"Enemy" + std::to_string(static_cast<int>(type) + 1)};
	Faction faction{Faction::EnemyTeam};

	Log::Info("spawn " + name + " (" + std::string{ToString(faction)} + ") uuid " + UuidUtils::GetStringUuid(uuid));

	DelayedSpawnStart(rect, health, name, faction, speed, uuid, type);

	return true;
}

void TankSpawner::SpawnPlayer(const ObjRectangle rect, const float speed, const int health, const Uuid uuid,
							  const TankType type)
{
	const bool isFirst = type == TankType::PLAYER1;
	const std::string name{isFirst ? "Player1" : "Player2"};
	Faction faction{Faction::PlayerTeam};

	Log::Info("spawn " + name + " (" + std::string{ToString(faction)} + ") uuid " + UuidUtils::GetStringUuid(uuid));

	DelayedSpawnStart(rect, health, name, faction, speed, uuid, type);
}

void TankSpawner::SpawnCoopBot(const ObjRectangle rect, const float speed, const int health, const Uuid uuid,
							   const TankType type)
{
	const std::string name{(type == TankType::COOP1 ? "CoopBot1" : "CoopBot2")};
	Faction faction{Faction::PlayerTeam};

	Log::Info("spawn " + name + " (" + std::string{ToString(faction)} + ") uuid " + UuidUtils::GetStringUuid(uuid));

	DelayedSpawnStart(rect, health, name, faction, speed, uuid, type);
}

void TankSpawner::RespawnEnemyTanks(const TankType type, const Uuid uuid,
									const std::optional<ObjRectangle> rect)
{
	const ObjRectangle spawnRect = rect.has_value() ? *rect : GetEnemyRandomPosX(type);
	const bool isSuccessSpawn = SpawnEnemy(spawnRect, uuid, type, _gameConfig.tankSpeed, _gameConfig.tankHealth);
	if (isSuccessSpawn && IsHost(_gameMode))
	{
		_events->EmitEvent(TankRespawnedEvent{.type = type, .uuid = uuid, .pos = FPoint{.x = spawnRect.x, .y = spawnRect.y}});
	}
}

//TODO: write unit test for bot change direction if faced obstacle
ObjRectangle TankSpawner::GetPlayerRandomPosX(const bool isFirst) const
{
	const float battleFieldSizeX{static_cast<float>(_gameConfig.battlefieldSize.x)};
	const float battleFieldSizeY{static_cast<float>(_gameConfig.battlefieldSize.y)};
	// const float gridOffset{_gameConfig.gridOffset};
	const float tankSize{_gameConfig.tankSize};

	const std::pair spawnRangePlayer1{0.f, battleFieldSizeX / 2.f - tankSize * 3.25f};
	const std::pair spawnRangePlayer2{battleFieldSizeX / 2.f + tankSize * 2.25f, battleFieldSizeX - tankSize};
	auto [minX, maxX]{isFirst ? spawnRangePlayer1 : spawnRangePlayer2};

	const std::uniform_real_distribution distRandId{minX, maxX};
	const float randomX = RandUtils::GetRandNumber(distRandId);

	ObjRectangle rect{.x = -1.f, .y = -1.f, .w = tankSize, .h = tankSize};
	ObjRectangle spawnPos{.x = randomX, .y = battleFieldSizeY - tankSize, .w = tankSize, .h = tankSize};
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

void TankSpawner::RespawnPlayerTeam(const TankType type, const Uuid uuid,
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
		|| IsNetworkGame(_gameMode)
		|| (_gameMode == GameMode::CoopWithBot && isFirst))
	{
		SpawnPlayer(spawnRect, _gameConfig.tankSpeed, _gameConfig.tankHealth, uuid, type);
		if (IsHost(_gameMode))
		{
			_events->EmitEvent(TankRespawnedEvent{.type = type, .uuid = uuid, .pos = FPoint{.x = spawnRect.x, .y = spawnRect.y}});
		}
	}
	else if (UsesCoopBots(_gameMode))
	{
		SpawnCoopBot(spawnRect, _gameConfig.tankSpeed, _gameConfig.tankHealth, uuid,
					 isFirst ? TankType::COOP1 : TankType::COOP2);
	}
}

void TankSpawner::RespawnTank(const TankType type, const Uuid uuid, const std::optional<ObjRectangle> rect)
{
	switch (type)
	{
		case TankType::ENEMY1:
		case TankType::ENEMY2:
		case TankType::ENEMY3:
		case TankType::ENEMY4:
		{
			if (const bool isNetworkMirrored = rect.has_value();// server already decided when to spawn this tank
				isNetworkMirrored)
			{
				RespawnEnemyTanks(type, uuid, rect);
			}
			else if (TimeUtils::IsCooldownFinish(_enemySpawnTimer.activateTime, _enemySpawnTimer.cooldown))
			{
				_enemySpawnTimer.Reset();
				RespawnEnemyTanks(type, uuid, rect);
			}
			break;
		}
		case TankType::PLAYER1:
		case TankType::PLAYER2:
		case TankType::COOP1:
		case TankType::COOP2:
			RespawnPlayerTeam(type, uuid, rect);
			break;
	}
}

//TODO: maybe we don't need spawn on client at all and just move the textures and animation?
void TankSpawner::OnClientRespawn(const TankType type, const Uuid uuid, const ObjRectangle rect)
{
	RespawnTank(type, uuid, rect);
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
	const bool isNet = IsClient(_gameMode) || (IsHost(_gameMode) && !isFirst);

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
									const Faction faction, const float speed, const Uuid uuid, const TankType type)
{
	_delayedSpawns.push_back(DelayedTankSpawn{.uuid = uuid,
											  .type = type,
											  .rect = rect,
											  .health = health,
											  .name = name,
											  .faction = faction,
											  .speed = speed});

	_events->EmitEvent(TankSpawnEvent{.uuid = uuid});

	//NOTE: the burst is also the countdown - the tank lands when its last frame is done
	_events->EmitEvent(AnimationCreateTankSpawnEvent{.rect = rect, .name = name, .uuid = uuid});
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
									.faction = params.faction};

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
		_events->EmitEvent(BonusReApplyEvent{.uuid = params.uuid, .name = params.name, .faction = params.faction});

		if (IsHost(_gameMode))
		{
			_events->EmitEvent(TankSpawnCompletedEvent{.uuid = params.uuid});
		}
	}
}
