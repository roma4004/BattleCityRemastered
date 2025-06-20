#pragma once

#include <chrono>
#include <memory>
#include <random>
#include <vector>
#include <boost/uuid/uuid.hpp>

enum GameMode : char8_t;
enum BonusType : char8_t;
struct UPoint;
struct ObjRectangle;
class BaseObj;
class EventSystem;

class BonusSpawner final
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

	std::string _name{"BonusSpawner"};
	GameMode _gameMode{};
	int _bonusSize{0};

	std::shared_ptr<EventSystem> _events{nullptr};

	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};

	std::mt19937 _gen;
	std::uniform_int_distribution<> _distSpawnPosY;
	std::uniform_int_distribution<> _distSpawnPosX;
	std::uniform_int_distribution<> _distSpawnType;
	std::uniform_int_distribution<> _distRandColor;

	milliseconds _cooldownBonusSpawn{std::chrono::seconds{6}}; // Bonus spawn time
	std::chrono::system_clock::time_point _lastTimeSpawn;

	void Subscribe();
	void SubscribeAsHost();
	void SubscribeAsClient();

	void Unsubscribe() const;
	void UnsubscribeAsHost() const;
	void UnsubscribeAsClient() const;

	void Update();

public:
	BonusSpawner(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	             UPoint windowSize, int sideBarWidth = 175, int bonusSize = 36);//TODO: bonus size should be in bonus.h

	~BonusSpawner();

	void SpawnRandomBonus(ObjRectangle rect);
	void SpawnBonus(ObjRectangle rect, int color, BonusType type, buuid uuid = {});
};
