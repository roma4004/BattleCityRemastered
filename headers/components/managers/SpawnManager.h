#pragma once

#include <memory>
#include <string>
#include <vector>

struct UPoint;
class BaseObj;
class EventSystem;
class DelayedSpawnManager;
class BonusSpawner;
class ObstacleSpawner;
class TankSpawner;
class GameConfig;

class SpawnManager
{
	std::string _name{};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<DelayedSpawnManager> _delayedSpawnManager{nullptr};
	std::shared_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::unique_ptr<TankSpawner> _tankSpawner{nullptr};

public:
	SpawnManager(const std::shared_ptr<EventSystem>& events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
				 GameConfig& gameConfig);

	~SpawnManager();
};
