#include "utils/Timer.h"

Timer::Timer() {}

Timer::Timer(const milliseconds cooldown, const std::chrono::system_clock::time_point activateTime)
	: cooldown{cooldown},
	  activateTime{activateTime},
	  isActive{true} {}

Timer::~Timer() = default;
