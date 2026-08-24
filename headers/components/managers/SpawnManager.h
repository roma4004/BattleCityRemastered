#pragma once

#include "components/EventSystem.h"
#include <memory>
#include <vector>

struct UPoint;
struct GameModeChangedToEvent;
class BaseObj;
class EventSystem;
class DelayedSpawnManager;
class FortressManager;
class BonusSpawner;
class ObstacleSpawner;
class TankSpawner;
class RespawnManager;
class GameConfig;

class SpawnManager
{
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};
	GameConfig& _gameConfig;
	std::vector<EventSubscription> _subs{};

	std::shared_ptr<DelayedSpawnManager> _delayedSpawnManager{nullptr};
	std::unique_ptr<FortressManager> _fortressManager{nullptr};
	std::shared_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::unique_ptr<RespawnManager> _respawnManager{nullptr};
	std::unique_ptr<TankSpawner> _tankSpawner{nullptr};

	void Subscribe();
	void CreateSpawners();
	void OnGameModeChangedTo(const GameModeChangedToEvent&);

public:
	SpawnManager(const std::shared_ptr<EventSystem>& events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
				 GameConfig& gameConfig);

	~SpawnManager();
};
