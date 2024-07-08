#pragma once

#include "Bonus.h"
#include "../BaseObj.h"
#include "../EventSystem.h"
#include "../Point.h"

#include <chrono>
#include <memory>

class BonusTimer : public Bonus
{
	void Subscribe();
	void Unsubscribe() const;

public:
	BonusTimer(const Rectangle& rect, int* windowBuffer, UPoint windowSize, std::shared_ptr<EventSystem> events,
	                int bonusDurationTimeSec, int lifeTimeSec, int color = 0x00ff00);

	~BonusTimer() override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	void PickUpBonus(const std::string& author, const std::string& fraction) override;
};
