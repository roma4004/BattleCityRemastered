#include "application/GameConfig.h"
#include "application/Simulation.h"
#include "components/EventSystem.h"
#include "components/events/TimingEvents.h"
#include "enums/GameMode.h"
#include <gtest/gtest.h>
#include <cstddef>
#include <memory>
#include <vector>

// movement is speed * deltaTime, so a step following the frame would land the same input differently per machine
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

// a frame worth exactly one step: the world advances once, by the fixed step and not by the frame's length
TEST_F(SimulationStepTest, AFrameOfOneStepAdvancesTheWorldOnce)
{
	Frame(kStep);

	ASSERT_EQ(1u, _steps.size());
	EXPECT_DOUBLE_EQ(kStep, _steps.front());
}

// a frame three steps long is paid in three equal steps, not one long one
TEST_F(SimulationStepTest, AFrameOfThreeStepsAdvancesTheWorldThreeTimes)
{
	Frame(kStep * 3.0);

	ASSERT_EQ(3u, _steps.size());
	for (const double step: _steps)
	{
		EXPECT_DOUBLE_EQ(kStep, step);
	}
}

// half a step moves nothing and is kept; the next half completes it
TEST_F(SimulationStepTest, AFrameShorterThanAStepIsCarriedIntoTheNext)
{
	Frame(kStep / 2.0);
	EXPECT_TRUE(_steps.empty()) << "half a step moved the world";

	Frame(kStep / 2.0);
	EXPECT_EQ(1u, _steps.size());
}

// vsync never lands exactly on the step - without the snap the drift alternates zero and two steps, a stutter
TEST_F(SimulationStepTest, JitterAroundTheStepStaysOneStepPerFrame)
{
	constexpr double kJitter{kStep / 40.0};

	//NOTE: counted per frame, not in total - the drift keeps the sum but pays it as an empty frame and a double one
	for (int frame{}; frame < 60; ++frame)
	{
		const std::size_t before{_steps.size()};
		Frame(frame % 2 == 0 ? kStep - kJitter : kStep + kJitter);

		EXPECT_EQ(1u, _steps.size() - before) << "frame " << frame << " did not advance exactly once";
	}
}

// catch-up is capped: each step is a full frame of movement and shooting, so a stall must not return as a burst
TEST_F(SimulationStepTest, ALongStallDoesNotBurstIntoUnboundedCatchUp)
{
	Frame(kStep * 100.0);

	EXPECT_EQ(4u, _steps.size());
}

// a client runs no entity logic; the mode is set on the config, not applied - applying it would dial a server
TEST_F(SimulationStepTest, AClientTakesNoStepsAtAll)
{
	_gameConfig.gameMode = GameMode::PlayAsClient;

	Frame(kStep * 3.0);

	EXPECT_TRUE(_steps.empty());
}
