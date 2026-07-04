#include "utils/Timer.h"
#include "utils/TimeUtils.h"

Timer::Timer() = default;

Timer::Timer(const milliseconds cooldown, const std::chrono::system_clock::time_point activateTime)
	: cooldown{cooldown}
	, activateTime{activateTime}
	, isActive{true} {}

Timer::~Timer() = default;

bool Timer::IsCooldownFinish() const { return TimeUtils::IsCooldownFinish(activateTime, cooldown); }

void Timer::Reset()
{
	activateTime = std::chrono::system_clock::now();
	isActive = true;
}

void Timer::Reset(const milliseconds newCooldown)
{
	cooldown = newCooldown;
	Reset();
}
