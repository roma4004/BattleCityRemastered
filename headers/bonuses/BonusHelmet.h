#pragma once

#include "Bonus.h"
#include "../BaseObj.h"
#include "../EventSystem.h"
#include "../Point.h"

#include <chrono>
#include <memory>

class BonusHelmet : public Bonus
{
	void Subscribe();
	void Unsubscribe() const;

public:
	BonusHelmet(const Rectangle& rect, int* windowBuffer, UPoint windowSize, std::shared_ptr<EventSystem> events,
	            int durationSec, int lifeTimeSec, int color);

	~BonusHelmet() override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	void PickUpBonus(const std::string& author, const std::string& fraction) override;
};
