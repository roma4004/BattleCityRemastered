#pragma once

#include "Brick.h"
#include "Iron.h"
#include "../BaseObj.h"
#include "../EventSystem.h"
#include "../Point.h"
#include "../interfaces/ITickUpdatable.h"

#include <chrono>
#include <memory>

class ObstacleAroundFortress : public BaseObj, ITickUpdatable
{
	Rectangle _rect;
	UPoint _windowSize{0, 0};
	int* _windowBuffer{nullptr};
	std::shared_ptr<EventSystem> _events;
	std::vector<std::shared_ptr<BaseObj>>* _allObjects;
	std::string _name;

	std::unique_ptr<Brick> _brick{nullptr};
	std::unique_ptr<Iron> _iron{nullptr};
	bool _isBrick{true};
	bool _isActiveShovel{false};
	std::chrono::system_clock::time_point _activateTimeShovel;
	int _cooldownShovelMs{0};

	void Subscribe();
	void Unsubscribe() const;

	void Draw() const override;
	void TickUpdate(float deltaTime) override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	void Hide();
public:
	ObstacleAroundFortress(const Rectangle& rect, int* windowBuffer, UPoint windowSize,
	                    const std::shared_ptr<EventSystem>& events, std::vector<std::shared_ptr<BaseObj>>* allObjects);

	~ObstacleAroundFortress() override;

	void BonusShovelSwitch();

	void TakeDamage(int damage) override;
};
