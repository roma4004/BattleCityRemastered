#pragma once

#include "FortressWall.h"
#include "../Point.h"
#include "../interfaces/ITickUpdatable.h"

#include <chrono>
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
	UPoint _windowSize;
	int _obstacleSize{0};
	int _lastSpawnId{-1};
	std::shared_ptr<int[]> _windowBuffer{nullptr};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};
	std::mt19937 _gen;
	std::uniform_int_distribution<> _distSpawnPosY;
	std::uniform_int_distribution<> _distSpawnPosX;
	std::uniform_int_distribution<> _distSpawnType;

	void Subscribe();

	void Unsubscribe() const;

	void TickUpdate(float deltaTime) override;

public:
	ObstacleSpawner(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	             std::shared_ptr<int[]> windowBuffer, UPoint windowSize, int sideBarWidth = 175, int obstacleSize = 36);

	~ObstacleSpawner() override;

	void SpawnRandomObstacle(ObjRectangle rect);
	void SpawnObstacle(ObjRectangle rect, ObstacleTypeId bonusType, int id = -1);

	template<typename TObstaclesType>
	void SpawnObstacles(const ObjRectangle& rect, int id);
};

// Include the template implementation
#include "ObstacleSpawner.tpp"
