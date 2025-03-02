#pragma once

#include "FortressWall.h"
#include "../GameMode.h"
#include "../application/Window.h"
#include "../interfaces/ITickUpdatable.h"

#include <memory>
#include <random>
#include <vector>

enum ObstacleTypeId : char8_t;
enum BonusTypeId : char8_t;
struct UPoint;
struct ObjRectangle;
class BaseObj;
class EventSystem;

class ObstacleSpawner final : public ITickUpdatable
{
	std::string _name{"ObstacleSpawner"};
	std::shared_ptr<EventSystem> _events{nullptr};
	int _obstacleSize{0};
	int _lastSpawnId{-1};
	std::shared_ptr<Window> _window{nullptr};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};
	std::mt19937 _gen;
	std::uniform_int_distribution<> _distSpawnPosY;
	std::uniform_int_distribution<> _distSpawnPosX;
	std::uniform_int_distribution<> _distSpawnType;
	GameMode _currentMode{Demo};

	void Subscribe();
	void SubscribeAsClient();

	void Unsubscribe() const;
	void UnsubscribeAsClient() const;

	void TickUpdate(float deltaTime) override;

public:
	ObstacleSpawner(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	                std::shared_ptr<Window> window, int sideBarWidth = 175, int obstacleSize = 36);

	~ObstacleSpawner() override;

	void SpawnRandomObstacle(ObjRectangle rect);
	void SpawnObstacle(ObjRectangle rect, ObstacleTypeId bonusType, int id = -1);

	template<typename TObstaclesType>
	void SpawnObstacles(const ObjRectangle& rect, int id);
};

// Include the template implementation
#include "ObstacleSpawner.tpp"
