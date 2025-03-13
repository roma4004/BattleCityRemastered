#pragma once

#include <chrono>

class TimeUtils final
{
public:
	static bool IsCooldownFinish(std::chrono::system_clock::time_point activateTime,
	                             std::chrono::milliseconds cooldown);
};
