#include "utils/Timer.h"
#include "utils/TimeUtils.h"
#include <algorithm>
#include <chrono>

Timer::Timer(const milliseconds newCooldown)
	: cooldown{newCooldown}
	, activateTime{TimeUtils::Now()}
	, isActive{true} {}

bool Timer::IsCooldownFinish() const noexcept { return IsCooldownFinish(TimeUtils::Now()); }

bool Timer::IsCooldownFinish(const TimeUtils::time_point& now) const noexcept
{
	return now - activateTime >= cooldown;
}

Timer::milliseconds Timer::TimeLeft(const TimeUtils::time_point& now) const noexcept
{
	return std::max(cooldown - std::chrono::duration_cast<milliseconds>(now - activateTime), milliseconds{});
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
