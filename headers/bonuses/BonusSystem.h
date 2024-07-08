#pragma once

#include "../BaseObj.h"
#include "../EventSystem.h"
#include "../Point.h"
#include "../interfaces/ITickUpdatable.h"

#include <chrono>
#include <memory>
#include <random>
#include <vector>

class BonusSystem : public ITickUpdatable
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

	int _cooldownBonusSpawn{60};
	std::chrono::system_clock::time_point _lastTimeBonusSpawn;

	void Subscribe();
	void Unsubscribe() const;

	void TickUpdate(float deltaTime) override;

public:
	BonusSystem(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	            int* windowBuffer, UPoint windowSize, int sideBarWidth = 175, int bonusSize = 36);

	~BonusSystem() override;
};
