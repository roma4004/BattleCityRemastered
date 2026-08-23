#include "utils/Timer.h"
#include "utils/TimeUtils.h"

Timer::Timer() = default;

Timer::Timer(const milliseconds newCooldown)
	: cooldown{newCooldown}
	, activateTime{TimeUtils::Now()}
	, isActive{true} {}

Timer::~Timer() = default;

bool Timer::IsCooldownFinish() const { return TimeUtils::IsCooldownFinish(activateTime, cooldown); }

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
