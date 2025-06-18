#include "../../headers/components/ObstacleSpawner.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/GameMode.h"
#include "../../headers/enums/ObstacleType.h"
#include "../../headers/obstacles/BrickWall.h"
#include "../../headers/obstacles/EagleTile.h"
#include "../../headers/obstacles/FortressWall.h"
#include "../../headers/obstacles/GrassTile.h"
#include "../../headers/obstacles/IceTile.h"
#include "../../headers/obstacles/SteelWall.h"
#include "../../headers/obstacles/WaterTile.h"
#include "../../headers/utils/UuidUtils.h"
#include <chrono>
#include <memory>

class BaseObj;
class EventSystem;

ObstacleSpawner::ObstacleSpawner(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                                 /*, const int sideBarWidth*/ const int obstacleSize)
	: _events{std::move(events)},
	  _obstacleSize{obstacleSize},
	  _allObjects{allObjects}

// _distSpawnPosY{0, static_cast<int>(_window->size.y) - obstacleSize},
// _distSpawnPosX{0, static_cast<int>(_window->size.x) - sideBarWidth - obstacleSize},
// _distSpawnType{None + 1, lastId - 1}
{
	// std::random_device rd;
	// _gen = std::mt19937(
	// static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count()) + rd());

	Subscribe();
}

ObstacleSpawner::~ObstacleSpawner()
{
	Unsubscribe();
}

void ObstacleSpawner::Subscribe()
{
	_events->AddListener<const GameMode>("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		_gameMode = newGameMode;
		_gameMode == PlayAsClient ? SubscribeAsClient() : UnsubscribeAsClient();
	});
}

void ObstacleSpawner::SubscribeAsClient()
{
	_events->AddListener<const ObjRectangle, const ObstacleType, const buuid&>(
			"ClientReceived_ObstacleSpawn", _name,
			[this](const ObjRectangle rect, const ObstacleType type, const buuid& uuid)
			{
				SpawnObstacle(rect, type, uuid);
			});
}

void ObstacleSpawner::Unsubscribe() const
{
	_events->RemoveListener<const GameMode>("GameModeChangedTo", _name);

	if (_gameMode == PlayAsClient)
	{
		UnsubscribeAsClient();
	}
}

void ObstacleSpawner::UnsubscribeAsClient() const
{
	_events->RemoveListener<const ObjRectangle, const ObstacleType, const buuid&>(
			"ClientReceived_ObstacleSpawn", _name);
}

void ObstacleSpawner::TickUpdate(const float /*deltaTime*/) {}

void ObstacleSpawner::SpawnObstacle(const ObjRectangle rect, const ObstacleType type, buuid uuid)
{
	if (uuid == UuidUtils::GetNilUuid())
	{
		uuid = UuidUtils::GetRandomUuid();
	}

	switch (type)
	{
		case Brick:
			SpawnObstacles<BrickWall>(rect, uuid);
			break;
		case Steel:
			SpawnObstacles<SteelWall>(rect, uuid);
			break;
		case Water:
			SpawnObstacles<WaterTile>(rect, uuid);
			break;
		case Fortress:
			SpawnObstacles<FortressWall>(rect, uuid);
			break;
		case Eagle:
			SpawnObstacles<EagleTile>(rect, uuid);
			break;
		case Grass:
			SpawnObstacles<GrassTile>(rect, uuid);
			break;
		case Ice:
			SpawnObstacles<IceTile>(rect, uuid);
			break;
		default:
			break;
	}
}

/*void ObstacleSpawner::SpawnRandomObstacle(const ObjRectangle rect)
{
	const auto obstacleType = static_cast<ObstacleType>(_distSpawnType(_gen));
	SpawnObstacle(rect, obstacleType);
}*/
