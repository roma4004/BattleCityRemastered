#include "../../headers/obstacles/ObstacleSpawner.h"
#include "../../headers/EventSystem.h"
#include "../../headers/GameMode.h"
#include "../../headers/obstacles/BrickWall.h"
#include "../../headers/obstacles/FortressWall.h"
#include "../../headers/obstacles/ObstacleTypeId.h"
#include "../../headers/obstacles/SteelWall.h"
#include "../../headers/obstacles/WaterTile.h"
#include "../../headers/utils/ColliderUtils.h"
#include "../../headers/utils/TimeUtils.h"

#include <chrono>
#include <memory>

class BaseObj;
class EventSystem;

ObstacleSpawner::ObstacleSpawner(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                                 std::shared_ptr<int[]> windowBuffer, UPoint windowSize, const int sideBarWidth,
                                 const int obstacleSize)
	: _events{std::move(events)},
	  _windowSize{std::move(windowSize)},
	  _obstacleSize{obstacleSize},
	  _windowBuffer{std::move(windowBuffer)},
	  _allObjects{allObjects},
	  _distSpawnPosY{0, static_cast<int>(_windowSize.y) - obstacleSize},
	  _distSpawnPosX{0, static_cast<int>(_windowSize.x) - sideBarWidth - obstacleSize},
	  _distSpawnType{None + 1, lastId - 1}
{
	std::random_device rd;
	_gen = std::mt19937(
			static_cast<unsigned int>(std::chrono::high_resolution_clock::now().time_since_epoch().count()) + rd());

	Subscribe();
}

ObstacleSpawner::~ObstacleSpawner()
{
	Unsubscribe();
}

void ObstacleSpawner::Subscribe()
{
	if (_events == nullptr)
	{
		return;
	}

	_events->AddListener("SpawnerReset", _name, [this]() { this->_lastSpawnId = -1; });

	_events->AddListener<const GameMode>("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		_currentMode = newGameMode;
		if (_currentMode == PlayAsClient)
		{
			SubscribeAsClient();
		}
		else
		{
			UnsubscribeAsClient();
		}
	});
}

void ObstacleSpawner::SubscribeAsClient()
{
	//TODO: subscribe on game mode change, to update gameMode for spawning obstacle
	_events->AddListener<const FPoint, const ObstacleTypeId, const int>(
			"ClientReceived_ObstacleSpawn", _name,
			[this](const FPoint spawnPos, const ObstacleTypeId type, const int id)
			{
				const auto size = static_cast<float>(_obstacleSize);
				const ObjRectangle rect{.x = spawnPos.x, .y = spawnPos.y, .w = size, .h = size};
				SpawnObstacle(rect, type, id);
			});
}

void ObstacleSpawner::Unsubscribe() const
{
	if (_events == nullptr)
	{
		return;
	}

	_events->RemoveListener("SpawnerReset", _name);

	if (_currentMode == PlayAsClient)
	{
		UnsubscribeAsClient();
	}
}

void ObstacleSpawner::UnsubscribeAsClient() const
{
	_events->RemoveListener<const FPoint, const BonusTypeId, const int>("ClientReceived_ObstacleSpawn", _name);
}

void ObstacleSpawner::TickUpdate(const float /*deltaTime*/) {}

void ObstacleSpawner::SpawnObstacle(const ObjRectangle rect, const ObstacleTypeId bonusType, const int id)
{
	if (id != -1)
	{
		_lastSpawnId = id;
	}
	else
	{
		++_lastSpawnId;
	}

	switch (bonusType)
	{
		case None:
			break;
		case Brick:
			SpawnObstacles<BrickWall>(rect, _lastSpawnId);
			break;
		case Steel:
			SpawnObstacles<SteelWall>(rect, _lastSpawnId);
			break;
		case Water:
			SpawnObstacles<WaterTile>(rect, _lastSpawnId);
			break;
		case Fortress:
			SpawnObstacles<FortressWall>(rect, _lastSpawnId);
		default:
			break;
	}
}

void ObstacleSpawner::SpawnRandomObstacle(const ObjRectangle rect)
{
	const auto obstacleType = static_cast<ObstacleTypeId>(_distSpawnType(_gen));
	SpawnObstacle(rect, obstacleType);
}
