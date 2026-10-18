#include "utils/TimeUtils.h"

namespace
{
TimeUtils::time_point gameNow{};
}// namespace

TimeUtils::time_point TimeUtils::Now() noexcept { return gameNow; }

void TimeUtils::Advance(const clock::duration elapsed) noexcept { gameNow += elapsed; }
