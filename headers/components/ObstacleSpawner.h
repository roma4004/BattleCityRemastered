#pragma once

#include "Point.h"
#include <boost/uuid/uuid.hpp>

enum class GameMode : char8_t;
enum class ObstacleType : char8_t;
enum class BonusType : char8_t;
struct ObjRectangle;
class BaseObj;
class EventSystem;

class ObstacleSpawner final
{
	using buuid = boost::uuids::uuid;

	std::string _name{"ObstacleSpawner"};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};
	GameMode _gameMode{};
	UPoint _windowSize;
	// std::uniform_int_distribution<> _distSpawnPosY;
	// std::uniform_int_distribution<> _distSpawnPosX;
	// std::uniform_int_distribution<> _distSpawnType;

	void Subscribe();
	void SubscribeAsClient();

	void Unsubscribe() const;
	void UnsubscribeAsClient() const;

	void LoadMap() const;
	void SpawnObstacle(ObjRectangle rect, ObstacleType type, buuid uuid = {});
	// void SpawnRandomObstacle(ObjRectangle rect);

public:
	ObstacleSpawner(const std::shared_ptr<EventSystem>& events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
					UPoint windowSize /*, int sideBarWidth = 175*/);

	~ObstacleSpawner();
};
