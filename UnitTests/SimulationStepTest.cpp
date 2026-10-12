#include "application/GameConfig.h"
#include "application/Simulation.h"
#include "components/EventSystem.h"
#include "components/events/TimingEvents.h"
#include "enums/GameMode.h"
#include <gtest/gtest.h>
#include <array>
#include <cstddef>
#include <memory>
#include <string_view>
#include <vector>

// movement is speed * deltaTime, so a step following the frame would land the same input differently per machine
class SimulationStepTest : public testing::Test
{
protected:
	static constexpr double kStep{1.0 / 60.0};

	std::shared_ptr<EventSystem> _events{std::make_shared<EventSystem>()};
	std::vector<double> _steps{};
	std::vector<EventSubscription> _subs{};

	void SetUp() override
	{
		_subs.push_back(_events->AddListener([this](const TickUpdateEvent& event)
		{
			_steps.push_back(event.deltaTime);
		}));
	}
};

// the world steps at a fixed rate whatever the frame: a long frame in equal steps, a short one carried, a stall capped
TEST_F(SimulationStepTest, TheWorldAdvancesInFixedSteps)
{
	struct Frames final
	{
		std::string_view why;
		std::vector<double> lengths;
		std::size_t steps{};
		GameMode mode{};
	};

	//NOTE: a client's mode is set on the config, not applied - applying it would dial a server
	const std::array frames{
			Frames{.why = "one step", .lengths = {kStep}, .steps = 1u},
			Frames{.why = "three steps in one frame", .lengths = {kStep * 3.0}, .steps = 3u},
			Frames{.why = "half a step is carried into the next", .lengths = {kStep / 2.0, kStep / 2.0}, .steps = 1u},
			Frames{.why = "a stall does not burst into catch-up", .lengths = {kStep * 100.0}, .steps = 4u},
			Frames{.why = "a client runs no entity logic", .lengths = {kStep * 3.0}, .mode = GameMode::PlayAsClient}};
	for (const Frames& row: frames)
	{
		SCOPED_TRACE(row.why);
		GameConfig gameConfig{};
		gameConfig.gameMode = row.mode;
		Simulation simulation{_events, gameConfig};
		_steps.clear();

		for (const double length: row.lengths)
		{
			_events->EmitEvent(DeltaTimeEvent{.deltaTime = length});
			simulation.Tick();
		}

		ASSERT_EQ(_steps.size(), row.steps);
		for (const double step: _steps)
		{
			EXPECT_DOUBLE_EQ(kStep, step);
		}
	}
}

// vsync never lands exactly on the step - without the snap the drift alternates zero and two steps, a stutter
TEST_F(SimulationStepTest, JitterAroundTheStepStaysOneStepPerFrame)
{
	constexpr double kJitter{kStep / 40.0};
	GameConfig gameConfig{};
	Simulation simulation{_events, gameConfig};

	//NOTE: counted per frame, not in total - the drift keeps the sum but pays it as an empty frame and a double one
	for (int frame{}; frame < 60; ++frame)
	{
		const std::size_t before{_steps.size()};
		_events->EmitEvent(DeltaTimeEvent{.deltaTime = frame % 2 == 0 ? kStep - kJitter : kStep + kJitter});
		simulation.Tick();

		EXPECT_EQ(1u, _steps.size() - before) << "frame " << frame << " did not advance exactly once";
	}
}
