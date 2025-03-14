#include "../../headers/utils/TimeUtils.h"

bool TimeUtils::IsCooldownFinish(const std::chrono::system_clock::time_point& activateTime,
                                 const std::chrono::milliseconds& cooldown)
{
	return std::chrono::system_clock::now() - activateTime >= cooldown;
}
