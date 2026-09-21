#pragma once

#include "utils/TimeUtils.h"
#include <chrono>

struct Timer
{
	using milliseconds = std::chrono::milliseconds;

	milliseconds cooldown{};
	TimeUtils::time_point activateTime{};
	bool isActive{};

	Timer() = default;

	explicit Timer(milliseconds newCooldown);

	[[nodiscard]] bool IsCooldownFinish() const noexcept;

	//NOTE: one reading for a whole sweep - asking twice lets a deadline fall between the two answers
	[[nodiscard]] bool IsCooldownFinish(const TimeUtils::time_point& now) const noexcept;

	void Reset();
	void Reset(milliseconds newCooldown);
};
