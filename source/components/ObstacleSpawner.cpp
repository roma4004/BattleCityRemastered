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

class BaseObj;
class EventSystem;

ObstacleSpawner::ObstacleSpawner(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                                 std::shared_ptr<Window> window, const int sideBarWidth, const int obstacleSize)
	: _events{std::move(events)},
	  _obstacleSize{obstacleSize},
	  _window{std::move(window)},
	  _allObjects{allObjects},
	  _distSpawnPosY{0, static_cast<int>(_window->size.y) - obstacleSize},
	  _distSpawnPosX{0, static_cast<int>(_window->size.x) - sideBarWidth - obstacleSize},
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
	_events->AddListener("Reset", _name, [this]() { this->_lastSpawnId = -1; });

	_events->AddListener<const GameMode>("GameModeChangedTo", _name, [this](const GameMode newGameMode)
	{
		_gameMode = newGameMode;
		_gameMode == PlayAsClient ? SubscribeAsClient() : UnsubscribeAsClient();
	});
}

void ObstacleSpawner::SubscribeAsClient()
{
	_events->AddListener<const FPoint, const ObstacleType, const int>(
			"ClientReceived_ObstacleSpawn", _name,
			[this](const FPoint spawnPos, const ObstacleType type, const int id)
			{
				const auto size = static_cast<float>(_obstacleSize);
				const ObjRectangle rect{.x = spawnPos.x, .y = spawnPos.y, .w = size, .h = size};
				SpawnObstacle(rect, type, id);
			});
}

void ObstacleSpawner::Unsubscribe() const
{
	_events->RemoveListener("Reset", _name);

	_events->RemoveListener<const GameMode>("GameModeChangedTo", _name);

	if (_gameMode == PlayAsClient)
	{
		UnsubscribeAsClient();
	}
}

void ObstacleSpawner::UnsubscribeAsClient() const
{
	_events->RemoveListener<const FPoint, const BonusType, const int>("ClientReceived_ObstacleSpawn", _name);
}

void ObstacleSpawner::TickUpdate(const float /*deltaTime*/) {}

void ObstacleSpawner::SpawnObstacle(const ObjRectangle rect, const ObstacleType bonusType, const int id)
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
	const auto obstacleType = static_cast<ObstacleType>(_distSpawnType(_gen));
	SpawnObstacle(rect, obstacleType);
}
