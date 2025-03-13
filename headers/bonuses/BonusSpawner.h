#pragma once

#include "../GameMode.h"
#include "../application/Window.h"
#include "../interfaces/ITickUpdatable.h"

#include <chrono>
#include <memory>
#include <random>
#include <vector>

enum BonusTypeId : char8_t;
struct UPoint;
struct ObjRectangle;
class BaseObj;
class EventSystem;

class BonusSpawner final : public ITickUpdatable
{
	std::string _name{"BonusSpawner"};
	GameMode _gameMode{Demo};
	int _bonusSize{0};
	int _lastSpawnId{-1};

	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<Window> _window{nullptr};

	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};

	std::mt19937 _gen;
	std::uniform_int_distribution<> _distSpawnPosY;
	std::uniform_int_distribution<> _distSpawnPosX;
	std::uniform_int_distribution<> _distSpawnType;
	std::uniform_int_distribution<> _distRandColor;

	std::chrono::milliseconds _cooldownBonusSpawn{std::chrono::seconds{60}};
	std::chrono::system_clock::time_point _lastTimeSpawn;

	void Subscribe();
	void SubscribeAsHost();
	void SubscribeAsClient();

	void Unsubscribe() const;
	void UnsubscribeAsHost() const;
	void UnsubscribeAsClient() const;

	void TickUpdate(float deltaTime) override;

public:
	BonusSpawner(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	             std::shared_ptr<Window> window, int sideBarWidth = 175, int bonusSize = 36);

	~BonusSpawner() override;

	void SpawnRandomBonus(ObjRectangle rect);
	void SpawnBonus(ObjRectangle rect, int color, BonusTypeId bonusType, int id = -1);

	template<typename TBonusType>
	void SpawnBonus(ObjRectangle rect, int color, int id = -1);
};
