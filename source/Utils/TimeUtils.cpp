#include "utils/TimeUtils.h"

namespace
{
struct GameClock
{
	TimeUtils::clock::duration pausedTotal{0};
	TimeUtils::time_point pauseStartedAt{};
	bool isPaused{false};
};

GameClock gameClock{};
}

TimeUtils::time_point TimeUtils::Now()
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

bool TimeUtils::IsPaused() { return gameClock.isPaused; }

bool TimeUtils::IsCooldownFinish(const time_point& activateTime, const milliseconds& cooldown)
{
	return Now() - activateTime >= cooldown;
}
