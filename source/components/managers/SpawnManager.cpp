#include "components/managers/SpawnManager.h"
#include "components/BonusSpawner.h"
#include "components/EventSystem.h"
#include "components/ObstacleSpawner.h"
#include "components/TankSpawner.h"
#include "components/managers/DelayedSpawnManager.h"

//TODO: add spawn queue
SpawnManager::SpawnManager(const std::shared_ptr<EventSystem>& events,
                           std::vector<std::shared_ptr<BaseObj>>* allObjects, UPoint windowSize)
	: _name{"SpawnDelayManager"},
	  _events{events},
	  _delayedSpawnManager{std::make_unique<DelayedSpawnManager>(events)},
	  _bonusSpawner{std::make_unique<BonusSpawner>(events, allObjects, windowSize)},
	  _obstacleSpawner{std::make_unique<ObstacleSpawner>(events, allObjects, windowSize)},
	  _tankSpawner{std::make_unique<TankSpawner>(windowSize, allObjects, events)} {}

SpawnManager::~SpawnManager() = default;
