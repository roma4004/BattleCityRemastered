#include "entities/bonuses/BonusStatus.h"

BonusStatus::BonusStatus() {}

BonusStatus::BonusStatus(const bool isActive, const milliseconds cooldown,
                         const std::chrono::system_clock::time_point activateTime)
	: cooldown{cooldown},
	  activateTime{activateTime},
	  isActive{isActive} {}

BonusStatus::~BonusStatus() {}
