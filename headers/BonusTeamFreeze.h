#pragma once

#include "BaseObj.h"
#include "Bonus.h"
#include "EventSystem.h"
#include "Point.h"

#include <chrono>
#include <memory>

class BonusTeamFreeze : public Bonus
{
	void Subscribe();
	void Unsubscribe() const;

public:
	BonusTeamFreeze(const Rectangle& rect, int* windowBuffer, UPoint windowSize, std::shared_ptr<EventSystem> events,
	                int bonusDurationTimeSec, int lifeTimeSec, int color = 0x00ff00);

	~BonusTeamFreeze() override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

	void PickUpBonus(const std::string& author, const std::string& fraction) override;
};
