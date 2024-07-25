#pragma once

#include <chrono>

class TimeUtils final
{
public:
	static bool IsCooldownFinish(std::chrono::system_clock::time_point activateTime, int cooldownMs);
};
