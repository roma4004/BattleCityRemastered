#pragma once

#include "utils/TimeUtils.h"
#include <chrono>

struct Timer
{
	using milliseconds = std::chrono::milliseconds;

	milliseconds cooldown{0};
	TimeUtils::time_point activateTime{};
	bool isActive{false};

	Timer();

	explicit Timer(milliseconds newCooldown);

	~Timer();

	[[nodiscard]] bool IsCooldownFinish() const;

	void Reset();
	void Reset(milliseconds newCooldown);
};
