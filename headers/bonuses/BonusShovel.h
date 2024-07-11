#pragma once

#include "Bonus.h"
#include "../BaseObj.h"
#include "../EventSystem.h"
#include "../Point.h"

#include <memory>

class BonusShovel final : public Bonus
{
	void Subscribe();
	void Unsubscribe() const;

public:
	BonusShovel(const Rectangle& rect, int* windowBuffer, UPoint windowSize, std::shared_ptr<EventSystem> events,
	          int durationMs, int lifeTimeMs, int color);

	~BonusShovel() override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	void PickUpBonus(const std::string& author, const std::string& fraction) override;
};
