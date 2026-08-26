#pragma once

#include "components/EventSystem.h"
#include "utils/Uuid.h"
#include <vector>

enum class GameMode : char8_t;
enum class ObstacleType : char8_t;
enum class BonusType : char8_t;
struct ObjRectangle;
class BaseObj;
class EventSystem;
class GameConfig;
struct MatchStartedEvent;
struct LoadMapEvent;
struct SpawnObstacleEvent;
struct SpawnFortressWallEvent;
struct ObstacleSpawnedEvent;

class ObstacleSpawner final
{
	//TODO: to the level select, once there is more than one map
	static constexpr auto kMapPath{"Resources/Maps/level1.map"};

	std::shared_ptr<EventSystem> _events{nullptr};
	const GameConfig& _gameConfig;
	std::vector<EventSubscription> _subs{};
	GameMode _gameMode{};

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

	~ObstacleSpawner() = default;
};
