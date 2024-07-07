#pragma once

#include "BaseObj.h"
#include "EventSystem.h"
#include "Point.h"
#include "interfaces/ITickUpdatable.h"

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

	int _cooldownEnemyTimer{10};
	int _cooldownPlayerTimer{10};
	bool _isActiveEnemyTimer{false};
	bool _isActivePlayerTimer{false};
	std::chrono::system_clock::time_point _enemyTimerActivateTime;
	std::chrono::system_clock::time_point _playerTimerActivateTime;

	int _cooldownHelmet{10};
	bool _isActiveEnemyOneHelmet{false};
	bool _isActiveEnemyTwoHelmet{false};
	bool _isActiveEnemyThreeHelmet{false};
	bool _isActiveEnemyFourHelmet{false};
	bool _isActivePlayerOneHelmet{false};
	bool _isActivePlayerTwoHelmet{false};
	bool _isActiveCoopOneAIHelmet{false};
	bool _isActiveCoopTwoAIHelmet{false};
	std::chrono::system_clock::time_point _enemyOneHelmetActivateTime;
	std::chrono::system_clock::time_point _enemyTwoHelmetActivateTime;
	std::chrono::system_clock::time_point _enemyThreeHelmetActivateTime;
	std::chrono::system_clock::time_point _enemyFourHelmetActivateTime;
	std::chrono::system_clock::time_point _playerOneHelmetActivateTime;
	std::chrono::system_clock::time_point _playerTwoHelmetActivateTime;
	std::chrono::system_clock::time_point _coopOneAIHelmetActivateTime;
	std::chrono::system_clock::time_point _coopTwoAIHelmetActivateTime;

	void Subscribe();
	void Unsubscribe() const;

	[[nodiscard]] static bool IsCooldownFinish(std::chrono::system_clock::time_point activateTime, int cooldown);

	void TickUpdate(float deltaTime) override;

public:
	BonusSystem(std::shared_ptr<EventSystem> events, std::vector<std::shared_ptr<BaseObj>>* allObjects,
	            int* windowBuffer, UPoint windowSize, int sideBarWidth = 175, int bonusSize = 36);

	~BonusSystem() override;
};
