#include "utils/TimeUtils.h"
#include "utils/Timer.h"
#include "gtest/gtest.h"
#include <chrono>
#include <thread>

using namespace std::chrono_literals;

class TimeUtilsTest : public testing::Test
{
protected:
	void TearDown() override { TimeUtils::SetPaused(false); }
};

// pause, sleep 20ms, ask for the time: the game clock has not moved; release it and it runs again
TEST_F(TimeUtilsTest, GameClockFreezesWhilePaused)
{
	TimeUtils::SetPaused(true);
	const auto frozen{TimeUtils::Now()};
	std::this_thread::sleep_for(20ms);

	EXPECT_TRUE(TimeUtils::IsPaused());
	EXPECT_EQ(TimeUtils::Now(), frozen);

	TimeUtils::SetPaused(false);

	EXPECT_FALSE(TimeUtils::IsPaused());
	EXPECT_GE(TimeUtils::Now(), frozen);
}

// pause twice with a sleep in between, then release twice - the second call of each pair changes
// nothing, so a host echo arriving after a local pause cannot skip the clock forward
TEST_F(TimeUtilsTest, SetPausedIsIdempotent)
{
	TimeUtils::SetPaused(true);
	const auto frozen{TimeUtils::Now()};
	std::this_thread::sleep_for(20ms);
	TimeUtils::SetPaused(true);

	EXPECT_EQ(TimeUtils::Now(), frozen);

	TimeUtils::SetPaused(false);
	const auto resumed{TimeUtils::Now()};
	TimeUtils::SetPaused(false);

	EXPECT_GE(TimeUtils::Now(), resumed);
}

// start a 100ms cooldown, spend 250ms paused: it is still not finished, and only 120ms of running
// time finishes it
TEST_F(TimeUtilsTest, CooldownSkipsThePause)
{
	const Timer timer{100ms};

	TimeUtils::SetPaused(true);
	std::this_thread::sleep_for(250ms);
	TimeUtils::SetPaused(false);

	EXPECT_FALSE(timer.IsCooldownFinish());

	std::this_thread::sleep_for(120ms);

	EXPECT_TRUE(timer.IsCooldownFinish());
}
