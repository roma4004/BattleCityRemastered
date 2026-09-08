#include "application/GameConfig.h"
#include "application/Simulation.h"
#include "components/EventSystem.h"
#include "components/events/TimingEvents.h"
#include "enums/GameMode.h"
#include <gtest/gtest.h>
#include <cstddef>
#include <memory>
#include <vector>

// Movement is speed * deltaTime, so a step that followed the frame would make the same input land
// differently on a fast machine and a slow one.
class SimulationStepTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{std::make_shared<EventSystem>()};
	GameConfig _gameConfig{};
	Simulation _simulation{_events, _gameConfig};

	std::vector<double> _steps{};
	std::vector<EventSubscription> _subs{};

	void SetUp() override
	{
		_subs.push_back(_events->AddListener([this](const TickUpdateEvent& event)
		{
			_steps.push_back(event.deltaTime);
		}));
	}

	//NOTE: the delta arrives as an event, exactly as FramePerSecondManager sends it
	void Frame(const double deltaTime)
	{
		_events->EmitEvent(DeltaTimeEvent{.deltaTime = deltaTime});
		_simulation.Tick();
	}

	static constexpr double kStep{1.0 / 60.0};
};

TEST_F(SimulationStepTest, AFrameOfOneStepAdvancesTheWorldOnce)
{
	Frame(kStep);

	ASSERT_EQ(1u, _steps.size());
	EXPECT_DOUBLE_EQ(kStep, _steps.front());
}

// A slow frame is paid for in whole steps, not in one long one
TEST_F(SimulationStepTest, AFrameOfThreeStepsAdvancesTheWorldThreeTimes)
{
	Frame(kStep * 3.0);

	ASSERT_EQ(3u, _steps.size());
	for (const double step: _steps)
	{
		EXPECT_DOUBLE_EQ(kStep, step);
	}
}

// The remainder is carried, so two half-frames are one step and nothing is lost
TEST_F(SimulationStepTest, AFrameShorterThanAStepIsCarriedIntoTheNext)
{
	Frame(kStep / 2.0);
	EXPECT_TRUE(_steps.empty()) << "half a step moved the world";

	Frame(kStep / 2.0);
	EXPECT_EQ(1u, _steps.size());
}

// Vsync never lands exactly on the step. Without the snap the accumulator drifts and turns that
// jitter into an alternating zero-then-two pattern, which reads as a stutter on screen.
TEST_F(SimulationStepTest, JitterAroundTheStepStaysOneStepPerFrame)
{
	constexpr double kJitter{kStep / 40.0};

	//NOTE: counted per frame, not in total - the drifting accumulator ends up with the same sum,
	//it just pays it in an empty frame and a double one
	for (int frame{0}; frame < 60; ++frame)
	{
		const std::size_t before = _steps.size();
		Frame(frame % 2 == 0 ? kStep - kJitter : kStep + kJitter);

		EXPECT_EQ(1u, _steps.size() - before) << "frame " << frame << " did not advance exactly once";
	}
}

// Catching up is capped: every step is a full frame of movement and shooting, and a stall must not
// come back as a burst of them
TEST_F(SimulationStepTest, ALongStallDoesNotBurstIntoUnboundedCatchUp)
{
	Frame(kStep * 100.0);

	EXPECT_EQ(4u, _steps.size());
}

// The client mirrors host-driven updates and runs no entity logic of its own. The mode is set on
// the config rather than applied - applying it would dial a server that is not there
TEST_F(SimulationStepTest, AClientTakesNoStepsAtAll)
{
	_gameConfig.gameMode = GameMode::PlayAsClient;

	Frame(kStep * 3.0);

	EXPECT_TRUE(_steps.empty());
}
