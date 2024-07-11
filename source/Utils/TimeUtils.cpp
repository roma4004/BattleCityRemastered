#include "../../headers/utils/TimeUtils.h"

bool TimeUtils::IsCooldownFinish(const std::chrono::system_clock::time_point activateTime, const int cooldownMs)
{
	const auto activateTimeMs = std::chrono::duration_cast<std::chrono::milliseconds>(
			activateTime.time_since_epoch()).count();
	const auto currentMs = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::system_clock::now().time_since_epoch()).count();

	if (currentMs - activateTimeMs >= cooldownMs)
	{
		return true;
	}

	return false;
}
