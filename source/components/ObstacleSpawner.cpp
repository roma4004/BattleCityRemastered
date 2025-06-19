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

void ObstacleSpawner::SpawnObstacle(const ObjRectangle rect, const ObstacleType type, buuid uuid)
{
	if (uuid == UuidUtils::GetNilUuid())
	{
		uuid = UuidUtils::GetRandomUuid();
	}

	std::shared_ptr<BaseObj> obstacle{nullptr};

	switch (type)
	{
		case Brick:
			obstacle = std::make_shared<BrickWall>(rect, _events, uuid, _gameMode);
			break;
		case Steel:
			obstacle = std::make_shared<SteelWall>(rect, _events, uuid, _gameMode);
			break;
		case Water:
			obstacle = std::make_shared<WaterTile>(rect, _events, uuid, _gameMode);
			break;
		case Fortress:
			obstacle = std::make_shared<FortressWall>(rect, _events, _allObjects, uuid, _gameMode);
			break;
		case Eagle:
			obstacle = std::make_shared<EagleTile>(rect, _events, uuid, _gameMode);
			break;
		case Grass:
			obstacle = std::make_shared<GrassTile>(rect, _events, uuid, _gameMode);
			break;
		case Ice:
			obstacle = std::make_shared<IceTile>(rect, _events, uuid, _gameMode);
			break;
		default:
			break;
	}

	if (obstacle)
	{
		_allObjects->emplace_back(obstacle);
	}
}

/*void ObstacleSpawner::SpawnRandomObstacle(const ObjRectangle rect)
{
	const auto obstacleType = static_cast<ObstacleType>(_distSpawnType(_gen));
	SpawnObstacle(rect, obstacleType);
}*/
