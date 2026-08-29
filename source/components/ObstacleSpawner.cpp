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
#include "entities/obstacles/FortressWalls.h"
#include "entities/obstacles/BushTile.h"
#include "entities/obstacles/IceTile.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/obstacles/WaterTile.h"
#include "enums/GameMode.h"
#include "enums/ObstacleType.h"
#include "utils/UuidUtils.h"
#include <memory>

class BaseObj;

ObstacleSpawner::ObstacleSpawner(const std::shared_ptr<EventSystem>& events, const GameConfig& gameConfig)
	: _events{events}
	, _gameConfig{gameConfig}
	, _gameMode{gameConfig.gameMode}
{
	Subscribe();
}

void ObstacleSpawner::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &ObstacleSpawner::OnMatchStarted));
	_subs.push_back(_events->AddListener(this, &ObstacleSpawner::OnLoadMap));
	_subs.push_back(_events->AddListener(this, &ObstacleSpawner::OnSpawnObstacle));
	_subs.push_back(_events->AddListener(this, &ObstacleSpawner::OnSpawnFortressWall));

	if (IsClient(_gameMode))
	{
		_subs.push_back(_events->AddListener(this, &ObstacleSpawner::OnObstacleSpawned));
	}
}

void ObstacleSpawner::OnMatchStarted(const MatchStartedEvent&) const
{
	if (!IsClient(_gameMode))
	{
		LoadMap();
	}
}

void ObstacleSpawner::OnLoadMap(const LoadMapEvent&) const { LoadMap(); }

void ObstacleSpawner::OnSpawnObstacle(const SpawnObstacleEvent& event) { SpawnObstacle(event.rect, event.type); }

void ObstacleSpawner::OnSpawnFortressWall(const SpawnFortressWallEvent& event)
{
	SpawnFortressWall(event.rect, event.material);
}

void ObstacleSpawner::OnObstacleSpawned(const ObstacleSpawnedEvent& event)
{
	const double side{_gameConfig.gridOffset * ObstacleCellSpan(event.type)};
	SpawnObstacle(ObjRectangle{.x = event.pos.x, .y = event.pos.y, .w = side, .h = side}, event.type, event.uuid);
}

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
			SpawnFortressWall(rect, ObstacleType::Brick);
			return;
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

	if (IsHost(_gameMode))
	{
		_events->EmitEvent(ObstacleSpawnedEvent{.pos = FPoint{.x = rect.x, .y = rect.y}, .type = type, .uuid = uuid});
	}
}

void ObstacleSpawner::SpawnFortressWall(const ObjRectangle rect, const ObstacleType material)
{
	const Uuid uuid = UuidUtils::GetRandomUuid();

	std::shared_ptr<BaseObj> wall{nullptr};
	if (material == ObstacleType::Steel)
	{
		wall = std::make_shared<FortressSteelWall>(rect, _events, uuid, _gameMode);
	}
	else
	{
		wall = std::make_shared<FortressBrickWall>(rect, _events, uuid, _gameMode);
	}

	//NOTE: reported from here, not from the map branch - a wall no one told FortressManager about would
	//never be rebuilt by the shovel, and a rebuilt one has to replace what the spot points at
	_events->EmitEvent(FortressSpotRegisteredEvent{.rect = rect, .wall = wall});

	_events->EmitEvent(AddToSpawnQueueEvent{.obj = std::move(wall)});

	if (IsHost(_gameMode))
	{
		_events->EmitEvent(ObstacleSpawnedEvent{
				.pos = FPoint{.x = rect.x, .y = rect.y}, .type = material, .uuid = uuid});
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
