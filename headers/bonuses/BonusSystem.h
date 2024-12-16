#pragma once

#include "../Point.h"
#include "../interfaces/ITickUpdatable.h"

#include <chrono>
#include <memory>
#include <random>
#include <vector>

struct UPoint;
struct ObjRectangle;
class BaseObj;
class EventSystem;

class BonusSystem final : public ITickUpdatable
{
	std::string _name;
	std::shared_ptr<EventSystem> _events;
	UPoint _windowSize;
	int _bonusSize;
	int* _windowBuffer;
	std::vector<std::shared_ptr<BaseObj>>* _allObjects{nullptr};
	std::mt19937 _gen;
	std::uniform_int_distribution<> _distSpawnPosY;
	std::uniform_int_distribution<> _distSpawnPosX;
	std::uniform_int_distribution<> _distSpawnType;
	std::uniform_int_distribution<> _distRandColor;

	int _cooldownBonusSpawn{60000};//60 sec
	std::chrono::system_clock::time_point _lastTimeBonusSpawn;

	void Subscribe();
	void Unsubscribe() const;

	void TickUpdate(float deltaTime) override;

public:
	BonusSystem(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	            int* windowBuffer, UPoint windowSize, int sideBarWidth = 175, int bonusSize = 36);

	~BonusSystem() override;

	void SpawnRandomBonus(ObjRectangle rect);
	void SpawnBonus(ObjRectangle rect, int color, int bonusId);
};
