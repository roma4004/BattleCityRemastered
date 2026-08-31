#pragma once

#include "components/EventSystem.h"
#include <memory>
#include <vector>

struct UPoint;
struct GameModeChangedToEvent;
struct AddToSpawnQueueEvent;
struct PostTickUpdateEvent;
struct GameResetEvent;
class BaseObj;
class EventSystem;
class FortressManager;
class BonusSpawner;
class ObstacleSpawner;
class TankSpawner;
class RespawnManager;
class GameConfig;

class SpawnManager
{
	std::shared_ptr<EventSystem> _events{nullptr};
	//NOTE: the world itself - spawners and pawns hold a const& to it, this class is its only writer
	std::vector<std::shared_ptr<BaseObj>> _allObjects{};
	std::vector<std::shared_ptr<BaseObj>> _pendingSpawns{};
	const GameConfig& _gameConfig;
	std::vector<EventSubscription> _subs{};

	std::unique_ptr<FortressManager> _fortressManager{nullptr};
	std::shared_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::unique_ptr<RespawnManager> _respawnManager{nullptr};
	std::unique_ptr<TankSpawner> _tankSpawner{nullptr};

	void Subscribe();
	void CreateSpawners();
	void OnGameModeChangedTo(const GameModeChangedToEvent&);
	void OnAddToSpawnQueue(const AddToSpawnQueueEvent& event);
	void OnPostTickUpdate(const PostTickUpdateEvent&);
	void OnGameReset(const GameResetEvent&);
	void FlushSpawnQueue();
	void DisposeDeadObject();

public:
	SpawnManager(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);

	~SpawnManager();
};
