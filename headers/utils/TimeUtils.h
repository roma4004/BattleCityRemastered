#pragma once

#include <chrono>

class TimeUtils final
{
	using milliseconds = std::chrono::milliseconds;

public:
	using clock = std::chrono::steady_clock;
	using time_point = clock::time_point;

	[[nodiscard]] static time_point Now();

	static void SetPaused(bool isPaused);

	[[nodiscard]] static bool IsPaused();

	[[nodiscard]] static bool IsCooldownFinish(const time_point& activateTime, const milliseconds& cooldown);
};
