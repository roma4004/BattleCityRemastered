#include "application/GameConfig.h"
#include "application/Simulation.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/TimingEvents.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

// the next level travels a restart's events, and their order is all: the reset empties the world, the map fills it
class NextLevelFlowTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{std::make_shared<EventSystem>()};
	GameConfig _gameConfig{};
	Simulation _simulation{_events, _gameConfig};

	//NOTE: what the frame emitted, in the order it did
	std::vector<std::string> _order{};
	std::vector<EventSubscription> _subs{};

	void SetUp() override
	{
		_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::OnePlayer});

		_subs.push_back(_events->AddListener([this](const GameResetEvent&) { _order.emplace_back("reset"); }));
		_subs.push_back(_events->AddListener([this](const LoadMapEvent&) { _order.emplace_back("map"); }));
	}

	void WinAndAskForTheNextLevel() const
	{
		_events->EmitEvent(GameFinishedEvent{.state = GameState::Won});
		_events->EmitEvent(NextLevelRequestedEvent{});
		_events->EmitEvent(PostTickUpdateEvent{});
	}
};

// one reset, then the next level's map - a reset after the load empties the level, a second one tears it down twice
TEST_F(NextLevelFlowTest, TheNextLevelIsOneResetThenItsMap)
{
	WinAndAskForTheNextLevel();

	EXPECT_EQ(_order, (std::vector<std::string>{"reset", "map"}));
}

// a restart is a new run, not one more level of the old one
TEST_F(NextLevelFlowTest, ARestartStartsOverFromTheFirstStage)
{
	WinAndAskForTheNextLevel();
	ASSERT_EQ(_gameConfig.stageNumber, 2u) << "the control failed - the next level did not count";

	_events->EmitEvent(MatchRestartRequestedEvent{});

	EXPECT_EQ(_gameConfig.stageNumber, 1u);
}

// the ask is answered a step later, so every listener of it still sees the field it has to read
TEST_F(NextLevelFlowTest, TheFieldStandsUntilTheFrameEnds)
{
	_events->EmitEvent(GameFinishedEvent{.state = GameState::Won});
	_events->EmitEvent(NextLevelRequestedEvent{});

	EXPECT_TRUE(_order.empty()) << "the world was torn down inside the ask for the next level";
}
