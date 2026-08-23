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

// Check that the game clock stands still while paused
TEST_F(TimeUtilsTest, GameClockFreezesWhilePaused)
{
	TimeUtils::SetPaused(true);
	const auto frozen = TimeUtils::Now();
	std::this_thread::sleep_for(20ms);

	EXPECT_TRUE(TimeUtils::IsPaused());
	EXPECT_EQ(TimeUtils::Now(), frozen);

	TimeUtils::SetPaused(false);

	EXPECT_FALSE(TimeUtils::IsPaused());
	EXPECT_GE(TimeUtils::Now(), frozen);
}

// Check that repeated calls with the same state don't stack up extra offset
TEST_F(TimeUtilsTest, SetPausedIsIdempotent)
{
	TimeUtils::SetPaused(true);
	const auto frozen = TimeUtils::Now();
	std::this_thread::sleep_for(20ms);
	TimeUtils::SetPaused(true);

	EXPECT_EQ(TimeUtils::Now(), frozen);

	TimeUtils::SetPaused(false);
	const auto resumed = TimeUtils::Now();
	TimeUtils::SetPaused(false);

	EXPECT_GE(TimeUtils::Now(), resumed);
}

// Check that a cooldown doesn't burn down during the pause and still finishes afterwards
TEST_F(TimeUtilsTest, CooldownSkipsThePause)
{
	Timer timer{100ms};

	TimeUtils::SetPaused(true);
	std::this_thread::sleep_for(250ms);
	TimeUtils::SetPaused(false);

	EXPECT_FALSE(timer.IsCooldownFinish());

	std::this_thread::sleep_for(120ms);

	EXPECT_TRUE(timer.IsCooldownFinish());
}
