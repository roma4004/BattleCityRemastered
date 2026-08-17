#pragma once

#include "Point.h"
#include "components/EventSystem.h"
#include <boost/uuid/uuid.hpp>
#include <vector>

enum class GameMode : char8_t;
enum class ObstacleType : char8_t;
enum class BonusType : char8_t;
struct ObjRectangle;
class BaseObj;
class EventSystem;
class GameConfig;
struct GameModeChangedToEvent;
struct LoadMapEvent;
struct SpawnObstacleEvent;
struct ClientInObstacleSpawnEvent;

class ObstacleSpawner final
{
	using buuid = boost::uuids::uuid;

	//TODO: to the level select, once there is more than one map
	static constexpr auto kMapPath{"Resources/Maps/level1.map"};

	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};
	GameConfig& _gameConfig;
	std::vector<EventSubscription> _subs{};
	// Toggled at runtime on every GameModeChangedToEvent, independent of _subs's fixed
	// subscribe-once-at-construction lifetime.
	EventSubscription _clientSub{};
	GameMode _gameMode{};
	// std::uniform_int_distribution<> _distSpawnPosY;
	// std::uniform_int_distribution<> _distSpawnPosX;
	// std::uniform_int_distribution<> _distSpawnType;

	void Subscribe();
	void OnGameModeChangedTo(const GameModeChangedToEvent& event);
	void OnLoadMap(const LoadMapEvent&) const;
	void OnSpawnObstacle(const SpawnObstacleEvent& event);
	void SubscribeAsClient();
	void OnClientInObstacleSpawn(const ClientInObstacleSpawnEvent& event);

	void UnsubscribeAsClient();

	void LoadMap() const;
	void SpawnObstacle(ObjRectangle rect, ObstacleType type, buuid uuid = {});
	// void SpawnRandomObstacle(ObjRectangle rect);

public:
	ObstacleSpawner(const std::shared_ptr<EventSystem>& events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
					GameConfig& gameConfig);

	~ObstacleSpawner() = default;
};
