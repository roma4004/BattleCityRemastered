#pragma once

#include <chrono>

class TimeUtils final
{
public:
	[[nodiscard]] static bool IsCooldownFinish(const std::chrono::system_clock::time_point& activateTime,
	                                           const std::chrono::milliseconds& cooldown);
};
