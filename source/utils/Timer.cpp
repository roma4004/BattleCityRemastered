#include "utils/Timer.h"
#include "utils/TimeUtils.h"

Timer::Timer(const milliseconds newCooldown)
	: cooldown{newCooldown}
	, activateTime{TimeUtils::Now()}
	, isActive{true} {}

bool Timer::IsCooldownFinish() const { return IsCooldownFinish(TimeUtils::Now()); }

bool Timer::IsCooldownFinish(const TimeUtils::time_point& now) const
{
	return now - activateTime >= cooldown;
}

void Timer::Reset()
{
	activateTime = TimeUtils::Now();
	isActive = true;
}

void Timer::Reset(const milliseconds newCooldown)
{
	cooldown = newCooldown;
	Reset();
}
