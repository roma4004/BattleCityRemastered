#pragma once

#include "Bonus.h"
#include "../BaseObj.h"
#include "../EventSystem.h"
#include "../Point.h"

#include <memory>

class BonusHelmet final : public Bonus
{
	void Subscribe();
	void Unsubscribe() const;

public:
	BonusHelmet(const ObjRectangle& rect, int* windowBuffer, UPoint windowSize, std::shared_ptr<EventSystem> events,
	            int durationMs, int lifeTimeMs, int color);

	~BonusHelmet() override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	void PickUpBonus(const std::string& author, const std::string& fraction) override;
};
