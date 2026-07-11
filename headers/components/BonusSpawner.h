#pragma once

#include "utils/Timer.h"
#include <boost/uuid/uuid.hpp>
#include <random>

enum class GameMode : char8_t;
enum class BonusType : char8_t;
struct UPoint;
struct ObjRectangle;
class BaseObj;
class EventSystem;

class BonusSpawner final
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

	std::string _name{"BonusSpawner"};

	std::shared_ptr<EventSystem> _events{nullptr};

	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};

	std::uniform_int_distribution<> _distSpawnPosY{};
	std::uniform_int_distribution<> _distSpawnPosX{};
	std::uniform_int_distribution<> _distSpawnType{};

	Timer _spawnTimer;
	int _bonusSize{};
	GameMode _gameMode{};

	void Subscribe();
	void SubscribeAsHost();
	void SubscribeAsClient();

	void Unsubscribe() const;
	void UnsubscribeAsHost() const;
	void UnsubscribeAsClient() const;

	void Update();
	void Reset();

public:
	BonusSpawner(const std::shared_ptr<EventSystem>& events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
				 UPoint windowSize, int sideBarWidth = 175, int bonusSize = 36);//TODO: bonus size should be in bonus.h

	~BonusSpawner();

	void SpawnRandomBonus(ObjRectangle rect);

	void SpawnBonus(ObjRectangle rect, BonusType type, buuid uuid = {});//NOTE: for unit tests
};
