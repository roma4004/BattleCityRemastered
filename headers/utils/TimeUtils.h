#pragma once

#include <chrono>

class TimeUtils final
{
	using milliseconds = std::chrono::milliseconds;

public:
	[[nodiscard]] static bool IsCooldownFinish(const std::chrono::system_clock::time_point& activateTime,
											   const milliseconds& cooldown);
};
