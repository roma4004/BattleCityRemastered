#include "../../headers/components/ObstacleSpawner.h"
#include "../../headers/application/Window.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/GameMode.h"
#include "../../headers/enums/ObstacleType.h"
#include "../../headers/obstacles/BrickWall.h"
#include "../../headers/obstacles/FortressWall.h"
#include "../../headers/obstacles/SteelWall.h"
#include "../../headers/obstacles/WaterTile.h"

#include <chrono>
#include <memory>
#include <boost/uuid/nil_generator.hpp>
#include <boost/uuid/random_generator.hpp>

class BaseObj;
class EventSystem;

ObstacleSpawner::ObstacleSpawner(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                                 std::shared_ptr<Window> window,
                                 std::shared_ptr<IDrawable> textureManager/*, const int sideBarWidth*/,
                                 const int obstacleSize)
	: _events{std::move(events)},
	  _obstacleSize{obstacleSize},
	  _window{std::move(window)},
	  _allObjects{allObjects},
	  _textureManager{std::move(textureManager)}

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

void ObstacleSpawner::SpawnObstacle(ObjRectangle rect, const ObstacleType type, const buuid uuid)
{
	buuid spawnUuid;
	if (uuid != boost::uuids::nil_uuid())
	{
		spawnUuid = uuid;
	}
	else
	{
		static boost::uuids::random_generator uuidObstacleGenerator;
		spawnUuid = uuidObstacleGenerator();
	}

	switch (type)
	{
		case None:
			break;
		case Brick:
			SpawnObstacles<BrickWall>(std::move(rect), spawnUuid);
			break;
		case Steel:
			SpawnObstacles<SteelWall>(std::move(rect), spawnUuid);
			break;
		case Water:
			SpawnObstacles<WaterTile>(std::move(rect), spawnUuid);
			break;
		case Fortress:
			SpawnObstacles<FortressWall>(std::move(rect), spawnUuid);
		default:
			break;
	}
}

/*void ObstacleSpawner::SpawnRandomObstacle(ObjRectangle rect)
{
	const auto obstacleType = static_cast<ObstacleType>(_distSpawnType(_gen));
	SpawnObstacle(std::move(rect), obstacleType);
}*/
