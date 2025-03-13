#include "../../headers/utils/TimeUtils.h"

bool TimeUtils::IsCooldownFinish(const std::chrono::system_clock::time_point activateTime, const std::chrono::milliseconds cooldown)
{
	if (const auto currentTime = std::chrono::system_clock::now();
		currentTime - activateTime >= cooldown)
	{
		return true;
	}

	return false;
}
