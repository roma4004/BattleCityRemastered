#include "components/ObstacleSpawner.h"
#include "utils/Log.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/Map.h"
#include "components/events/SpawnEvents.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/EagleTile.h"
#include "entities/obstacles/FortressWall.h"
#include "entities/obstacles/BushTile.h"
#include "entities/obstacles/IceTile.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/obstacles/WaterTile.h"
#include "enums/GameMode.h"
#include "enums/ObstacleType.h"
#include "utils/UuidUtils.h"
#include <memory>

class BaseObj;

ObstacleSpawner::ObstacleSpawner(const std::shared_ptr<EventSystem>& events,
								 std::vector<std::shared_ptr<BaseObj>>* allObjects, GameConfig& gameConfig)
	: _allObjects{allObjects}
	, _events{events}
	, _gameConfig{gameConfig}
{
	Subscribe();
}

void ObstacleSpawner::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &ObstacleSpawner::OnGameModeChangedTo));
	_subs.push_back(_events->AddListener(this, &ObstacleSpawner::OnLoadMap));
	_subs.push_back(_events->AddListener(this, &ObstacleSpawner::OnSpawnObstacle));
}

void ObstacleSpawner::OnGameModeChangedTo(const GameModeChangedToEvent& event)
{
	_gameMode = event.mode;
	IsClient(_gameMode) ? SubscribeAsClient() : UnsubscribeAsClient();

	//NOTE: network game should wait until players connect, if local just load the map
	if (IsLocalGame(_gameMode))
	{
		LoadMap();
	}
}

void ObstacleSpawner::OnLoadMap(const LoadMapEvent&) const { LoadMap(); }

void ObstacleSpawner::OnSpawnObstacle(const SpawnObstacleEvent& event) { SpawnObstacle(event.rect, event.type); }

void ObstacleSpawner::SubscribeAsClient()
{
	_clientSub = _events->AddListener(this, &ObstacleSpawner::OnClientInObstacleSpawn);
}

void ObstacleSpawner::OnClientInObstacleSpawn(const ClientInObstacleSpawnEvent& event)
{
	SpawnObstacle(event.rect, event.type, event.uuid);
}

void ObstacleSpawner::UnsubscribeAsClient() { _clientSub = EventSubscription{}; }

void ObstacleSpawner::SpawnObstacle(const ObjRectangle rect, const ObstacleType type, Uuid uuid)
{
	if (uuid == UuidUtils::GetNilUuid())
	{
		uuid = UuidUtils::GetRandomUuid();
	}

	std::shared_ptr<BaseObj> obstacle{nullptr};

	switch (type)
	{
		case ObstacleType::Brick:
			obstacle = std::make_shared<BrickWall>(rect, _events, uuid, _gameMode);
			break;
		case ObstacleType::Steel:
			obstacle = std::make_shared<SteelWall>(rect, _events, uuid, _gameMode);
			break;
		case ObstacleType::Water:
			obstacle = std::make_shared<WaterTile>(rect, _events, uuid, _gameMode);
			break;
		case ObstacleType::Fortress:
			obstacle = std::make_shared<FortressWall>(rect, _events, _allObjects, uuid, _gameMode);
			break;
		case ObstacleType::Eagle:
			obstacle = std::make_shared<EagleTile>(rect, _events, uuid, _gameMode);
			break;
		case ObstacleType::Bush:
			obstacle = std::make_shared<BushTile>(rect, _events, uuid, _gameMode);
			break;
		case ObstacleType::Ice:
			obstacle = std::make_shared<IceTile>(rect, _events, uuid, _gameMode);
			break;
		default:
			break;
	}

	if (obstacle)
	{
		_events->EmitEvent(AddToSpawnQueueEvent{.obj = obstacle});
	}
}

/*void ObstacleSpawner::SpawnRandomObstacle(const ObjRectangle rect)
{
	const auto obstacleType = static_cast<ObstacleType>(RandUtils::GetRandNumber(_distSpawnType));
	SpawnObstacle(rect, obstacleType);
}*/

void ObstacleSpawner::LoadMap() const
{
	Map map{_events};
	if (const auto loaded = map.LoadFromFile(kMapPath);
		!loaded)
	{
		const MapError& error = loaded.error();
		const std::string where = error.line != 0u ? " (line " + std::to_string(error.line) + ')' : std::string{};
		Log::Error("cannot load map " + error.path.string() + where + ": " + error.reason);

		return;
	}

	//NOTE: the cell size comes from this, so it has to be settled before a single obstacle is placed -
	//the bus is synchronous, so by the time this returns _gameConfig already holds the new geometry
	_events->EmitEvent(MapLoadedEvent{.cols = map.GetCols(), .rows = map.GetRows()});

	map.CreateObstacles(_gameConfig.gridOffset);
}
