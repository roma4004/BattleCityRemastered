#include "components/managers/SpawnManager.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/EventSystem.h"
#include "components/ObstacleSpawner.h"
#include "components/TankSpawner.h"
#include "components/managers/DelayedSpawnManager.h"
#include "components/managers/RespawnManager.h"

SpawnManager::SpawnManager(const std::shared_ptr<EventSystem>& events,
						   std::vector<std::shared_ptr<BaseObj>>* allObjects, GameConfig& gameConfig)
	: _events{events}
	, _delayedSpawnManager{std::make_unique<DelayedSpawnManager>(events)}
	, _bonusSpawner{std::make_unique<BonusSpawner>(events, allObjects, gameConfig)}
	, _obstacleSpawner{std::make_unique<ObstacleSpawner>(events, allObjects, gameConfig)}
	, _respawnManager{std::make_unique<RespawnManager>(events)}
	, _tankSpawner{std::make_unique<TankSpawner>(gameConfig, allObjects, events)} {}

SpawnManager::~SpawnManager() = default;
