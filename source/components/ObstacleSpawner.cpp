#include "components/ObstacleSpawner.h"
#include "Point.h"
#include "components/EventSystem.h"
#include "components/Map.h"
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
								 std::vector<std::shared_ptr<BaseObj>>* allObjects,/*, const int sideBarWidth*/
								 UPoint windowSize)
	: _allObjects{allObjects}
	, _events{events}
	, _windowSize{windowSize}

// _distSpawnPosY{0, static_cast<int>(_window->size.y) - obstacleSize},
// _distSpawnPosX{0, static_cast<int>(_window->size.x) - sideBarWidth - obstacleSize},
// _distSpawnType{None + 1, lastId - 1}
{
	Subscribe();
}

ObstacleSpawner::~ObstacleSpawner()
{
	Unsubscribe();
}

void ObstacleSpawner::Subscribe()
{
	_events->AddListener("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		_gameMode = newGameMode;
		_gameMode == GameMode::PlayAsClient ? SubscribeAsClient() : UnsubscribeAsClient();
	});

	_events->AddListener("LoadMap", _name, [this]() { LoadMap(); });
	_events->AddListener("SpawnObstacle", _name, [this](const ObjRectangle rect, const ObstacleType type)
	{
		SpawnObstacle(rect, type);
	});
}

void ObstacleSpawner::SubscribeAsClient()
{
	_events->AddListener(
			"ClientReceived_ObstacleSpawn", _name,
			[this](const ObjRectangle rect, const ObstacleType type, const buuid& uuid)
			{
				SpawnObstacle(rect, type, uuid);
			});
}

void ObstacleSpawner::Unsubscribe() const { _events->RemoveAllListeners(_name); }

void ObstacleSpawner::UnsubscribeAsClient() const { _events->RemoveListener("ClientReceived_ObstacleSpawn", _name); }

void ObstacleSpawner::SpawnObstacle(const ObjRectangle rect, const ObstacleType type, buuid uuid)
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
		_allObjects->emplace_back(obstacle);
	}
}

/*void ObstacleSpawner::SpawnRandomObstacle(const ObjRectangle rect)
{
	const auto obstacleType = static_cast<ObstacleType>(RandUtils::GetRandNumber(_distSpawnType));
	SpawnObstacle(rect, obstacleType);
}*/

void ObstacleSpawner::LoadMap() const
{
	//Map creation
	const float gridOffset = static_cast<float>(_windowSize.y) / 50.f;
	//TODO: update window size via subscription onChange
	const Map field{_events};
	field.MapCreation(gridOffset);
}
