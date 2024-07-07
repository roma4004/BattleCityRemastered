#pragma once

#include "EventSystem.h"
#include "interfaces/ITickUpdatable.h"

#include <chrono>
#include <memory>

class BonusSystem : public ITickUpdatable
{
	const char* _name;
	std::shared_ptr<EventSystem> _events;

	int cooldownEnemyTeamFreeze{10};
	int cooldownPlayerTeamFreeze{10};
	bool isActiveEnemyTeamFreeze{false};
	bool isActivePlayerTeamFreeze{false};
	std::chrono::system_clock::time_point enemyTeamFreezeActivateTime;
	std::chrono::system_clock::time_point playerTeamFreezeActivateTime;

	int cooldownHelmet{10};
	bool isActiveEnemyOneHelmet{false};
	bool isActiveEnemyTwoHelmet{false};
	bool isActiveEnemyThreeHelmet{false};
	bool isActiveEnemyFourHelmet{false};
	bool isActivePlayerOneHelmet{false};
	bool isActivePlayerTwoHelmet{false};
	std::chrono::system_clock::time_point enemyOneHelmetActivateTime;
	std::chrono::system_clock::time_point enemyTwoHelmetActivateTime;
	std::chrono::system_clock::time_point enemyThreeHelmetActivateTime;
	std::chrono::system_clock::time_point enemyFourHelmetActivateTime;
	std::chrono::system_clock::time_point playerOneHelmetActivateTime;
	std::chrono::system_clock::time_point playerTwoHelmetActivateTime;

	void Subscribe();
	void Unsubscribe() const;

	[[nodiscard]] static bool IsCooldownFinish(std::chrono::system_clock::time_point activateTime, int cooldown);

	void TickUpdate(float deltaTime) override;

public:
	explicit BonusSystem(std::shared_ptr<EventSystem> events);

	~BonusSystem() override;


};
