#include "utils/TimeUtils.h"

namespace
{
struct GameClock
{
	TimeUtils::clock::duration pausedTotal{};
	TimeUtils::time_point pauseStartedAt{};
	bool isPaused{};
};

GameClock gameClock{};
}// namespace

TimeUtils::time_point TimeUtils::Now() noexcept
{
	return (gameClock.isPaused ? gameClock.pauseStartedAt : clock::now()) - gameClock.pausedTotal;
}

void TimeUtils::SetPaused(const bool isPaused)
{
	if (isPaused == gameClock.isPaused)
	{
		return;
	}

	gameClock.isPaused = isPaused;

	if (isPaused)
	{
		gameClock.pauseStartedAt = clock::now();
	}
	else
	{
		gameClock.pausedTotal += clock::now() - gameClock.pauseStartedAt;
	}
}

bool TimeUtils::IsPaused() noexcept { return gameClock.isPaused; }
