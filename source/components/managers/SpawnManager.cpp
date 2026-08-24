#include "components/managers/SpawnManager.h"
#include "application/GameConfig.h"
#include "components/events/GameModeEvents.h"
#include "components/BonusSpawner.h"
#include "components/EventSystem.h"
#include "components/ObstacleSpawner.h"
#include "components/TankSpawner.h"
#include "components/managers/DelayedSpawnManager.h"
#include "components/managers/FortressManager.h"
#include "components/managers/RespawnManager.h"

SpawnManager::SpawnManager(const std::shared_ptr<EventSystem>& events,
						   std::vector<std::shared_ptr<BaseObj>>* allObjects, const GameConfig& gameConfig)
	: _events{events}
	, _allObjects{allObjects}
	, _gameConfig{gameConfig}
	, _delayedSpawnManager{std::make_unique<DelayedSpawnManager>(events, gameConfig)}
	, _fortressManager{std::make_unique<FortressManager>(events, allObjects)}
{
	Subscribe();

	CreateSpawners();
}

void SpawnManager::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &SpawnManager::OnGameModeChangedTo));
}

SpawnManager::~SpawnManager() = default;

void SpawnManager::CreateSpawners()
{
	_bonusSpawner = std::make_unique<BonusSpawner>(_events, _allObjects, _gameConfig);
	_obstacleSpawner = std::make_unique<ObstacleSpawner>(_events, _allObjects, _gameConfig);
	_respawnManager = std::make_unique<RespawnManager>(_events, _gameConfig.gameMode);
	_tankSpawner = std::make_unique<TankSpawner>(_gameConfig, _allObjects, _events);
}

void SpawnManager::OnGameModeChangedTo(const GameModeChangedToEvent&) { CreateSpawners(); }
