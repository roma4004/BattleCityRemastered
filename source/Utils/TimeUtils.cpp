#include "../../headers/utils/TimeUtils.h"

bool TimeUtils::IsCooldownFinish(const std::chrono::system_clock::time_point activateTime, const int cooldown)
{
	const auto activateTimeSec =
			std::chrono::duration_cast<std::chrono::seconds>(activateTime.time_since_epoch()).count();
	const auto currentSec =
			std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now().time_since_epoch())
			.count();

	if (currentSec - activateTimeSec >= cooldown)
	{
		return true;
	}

	return false;
}
