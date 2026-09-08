#pragma once

#include "components/EventSystem.h"
#include "utils/Uuid.h"
#include <memory>
#include <vector>

enum class ObstacleType : char8_t;
enum class BonusType : char8_t;
struct ObjRectangle;
struct MatchStartedEvent;
struct LoadMapEvent;
struct SpawnObstacleEvent;
struct SpawnFortressWallEvent;
struct ObstacleSpawnedEvent;
class BaseObj;
class EventSystem;
class GameConfig;

class ObstacleSpawner final
{
	//TODO: to the level select, once there is more than one map
	static constexpr auto kMapPath{"Resources/Maps/level1.map"};

	std::shared_ptr<EventSystem> _events{nullptr};
	const GameConfig& _gameConfig;
	std::vector<EventSubscription> _subs{};

	void Subscribe();
	void OnMatchStarted(const MatchStartedEvent&) const;
	void OnLoadMap(const LoadMapEvent&) const;
	void OnSpawnObstacle(const SpawnObstacleEvent& event);
	void OnSpawnFortressWall(const SpawnFortressWallEvent& event);
	void OnObstacleSpawned(const ObstacleSpawnedEvent& event);

	void LoadMap() const;
	void SpawnObstacle(ObjRectangle rect, ObstacleType type, Uuid uuid = {});
	void SpawnFortressWall(ObjRectangle rect, ObstacleType material);
	// void SpawnRandomObstacle(ObjRectangle rect);

public:
	ObstacleSpawner(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig);
};
