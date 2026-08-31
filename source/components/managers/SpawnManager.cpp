#include "components/managers/SpawnManager.h"
#include "application/GameConfig.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/events/TimingEvents.h"
#include "components/BonusSpawner.h"
#include "components/EventSystem.h"
#include "components/ObstacleSpawner.h"
#include "components/TankSpawner.h"
#include "components/managers/FortressManager.h"
#include "components/managers/RespawnManager.h"
#include "entities/BaseObj.h"
#include <iterator>

SpawnManager::SpawnManager(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig)
	: _events{events}
	, _gameConfig{gameConfig}
	, _fortressManager{std::make_unique<FortressManager>(events, _allObjects)}
{
	Subscribe();

	CreateSpawners();
}

void SpawnManager::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &SpawnManager::OnGameModeChangedTo));
	_subs.push_back(_events->AddListener(this, &SpawnManager::OnAddToSpawnQueue));
	_subs.push_back(_events->AddListener(this, &SpawnManager::OnPostTickUpdate));
	_subs.push_back(_events->AddListener(this, &SpawnManager::OnGameReset));
}

SpawnManager::~SpawnManager() = default;

void SpawnManager::CreateSpawners()
{
	_bonusSpawner = std::make_unique<BonusSpawner>(_events, _allObjects, _gameConfig);
	_obstacleSpawner = std::make_unique<ObstacleSpawner>(_events, _gameConfig);
	_respawnManager = std::make_unique<RespawnManager>(_events, _gameConfig.gameMode);
	_tankSpawner = std::make_unique<TankSpawner>(_gameConfig, _allObjects, _events);
}

void SpawnManager::OnGameModeChangedTo(const GameModeChangedToEvent&) { CreateSpawners(); }

void SpawnManager::OnAddToSpawnQueue(const AddToSpawnQueueEvent& event)
{
	//NOTE: the bus is entered here rather than in a constructor - the object is whole by now, so
	//the call reaches the leaf, and a spawner emits nothing between this and the flush
	event.obj->Activate();
	_pendingSpawns.push_back(event.obj);
}

//NOTE: pour in, then sweep out - the two run back to back so nothing iterates the world in between
void SpawnManager::OnPostTickUpdate(const PostTickUpdateEvent&)
{
	FlushSpawnQueue();
	DisposeDeadObject();
}

void SpawnManager::OnGameReset(const GameResetEvent&)
{
	_allObjects.clear();
	_allObjects.reserve(1000);
	_pendingSpawns.clear();
}

void SpawnManager::FlushSpawnQueue()
{
	_allObjects.insert(_allObjects.end(), std::make_move_iterator(_pendingSpawns.begin()),
					   std::make_move_iterator(_pendingSpawns.end()));
	_pendingSpawns.clear();
}

void SpawnManager::DisposeDeadObject()
{
	const auto isDead = [](const std::shared_ptr<BaseObj>& obj)
	{
		return obj == nullptr || obj->GetIsAlive() == false;
	};

	//NOTE: leaving the world is its own step - a pooled bullet outlives this and must not keep
	//listening while it waits in the free list
	for (const std::shared_ptr<BaseObj>& obj: _allObjects)
	{
		if (obj != nullptr && obj->GetIsAlive() == false)
		{
			obj->Deactivate();
		}
	}

	std::erase_if(_allObjects, isDead);
}
