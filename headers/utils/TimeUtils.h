#pragma once

#include <chrono>

class TimeUtils final
{
public:
	using clock = std::chrono::steady_clock;
	using time_point = clock::time_point;

	[[nodiscard]] static time_point Now();

	static void SetPaused(bool isPaused);

	[[nodiscard]] static bool IsPaused();
};
