#pragma once

#include "Bonus.h"
#include "../BaseObj.h"
#include "../EventSystem.h"
#include "../Point.h"

#include <memory>

class BonusTank final : public Bonus
{
	void Subscribe();
	void Unsubscribe() const;

public:
	BonusTank(const Rectangle& rect, int* windowBuffer, UPoint windowSize, std::shared_ptr<EventSystem> events,
	          int durationSec, int lifeTimeSec, int color);

	~BonusTank() override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	void PickUpBonus(const std::string& author, const std::string& fraction) override;
};