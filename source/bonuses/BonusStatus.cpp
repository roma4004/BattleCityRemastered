#include "../../headers/bonuses/BonusStatus.h"

#include "../../headers/utils/TimeUtils.h"

BonusStatus::BonusStatus() {}

BonusStatus::BonusStatus(const bool isActive, const std::chrono::milliseconds cooldown,
                         const std::chrono::system_clock::time_point activateTime)
	: isActive{isActive},
	  cooldown{cooldown},
	  activateTime{activateTime} {}

BonusStatus::~BonusStatus() {}

void BonusStatus::UpdateBonus()
{
	if (isActive && TimeUtils::IsCooldownFinish(activateTime, cooldown))
	{
		isActive = false;
	}
}
