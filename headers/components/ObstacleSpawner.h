#pragma once

#include <memory>
#include <random>
#include <vector>
#include <boost/uuid/uuid.hpp>

enum class GameMode : char8_t;
enum class ObstacleType : char8_t;
enum class BonusType : char8_t;
struct UPoint;
struct ObjRectangle;
class BaseObj;
class EventSystem;

class ObstacleSpawner final
{
	using buuid = boost::uuids::uuid;

	std::string _name{"ObstacleSpawner"};
	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};
	std::shared_ptr<EventSystem> _events{nullptr};
	int _obstacleSize{0};
	GameMode _gameMode{};
	// std::uniform_int_distribution<> _distSpawnPosY;
	// std::uniform_int_distribution<> _distSpawnPosX;
	// std::uniform_int_distribution<> _distSpawnType;

	void Subscribe();
	void SubscribeAsClient();

	void Unsubscribe() const;
	void UnsubscribeAsClient() const;

	// void SpawnRandomObstacle(ObjRectangle rect);

public:
	ObstacleSpawner(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	                /*, int sideBarWidth = 175*/ int obstacleSize = 36);//TODO: obstacle size should be in obstacle.h

	~ObstacleSpawner();

	void SpawnObstacle(ObjRectangle rect, ObstacleType type, buuid uuid = {});
};
