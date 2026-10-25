#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/GameStatistics.h"
#include "components/ScoreBoard.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/managers/GameStateManager.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include "gtest/gtest.h"
#include <memory>

// the board follows the phase, not the score: each case sets a mode, announces a phase and reads if the board is up
class ScoreBoardTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<GameStatistics> _statistics{nullptr};
	std::unique_ptr<GameStateManager> _stateManager{nullptr};
	std::unique_ptr<ScoreBoard> _scoreBoard{nullptr};
	GameConfig _gameConfig{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_statistics = std::make_unique<GameStatistics>(_events);
		_stateManager = std::make_unique<GameStateManager>(_events);
		_scoreBoard = std::make_unique<ScoreBoard>(_events, *_statistics, _gameConfig);
	}
};

// a local match over, lost or won - the board goes up either way
TEST_F(ScoreBoardTest, AFinishedMatchShowsScore)
{
	for (const GameState result: {GameState::Over, GameState::Won})
	{
		SCOPED_TRACE(static_cast<int>(result));
		_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::OnePlayer});
		ASSERT_FALSE(_scoreBoard->IsShown());

		_events->EmitEvent(GameFinishedEvent{.state = result});

		EXPECT_TRUE(_scoreBoard->IsShown());
	}
}

//NOTE: a client never runs the win check, so the phase arriving from the server is all it has to go on
TEST_F(ScoreBoardTest, HostAnnouncedGameOverShowsScoreOnClient)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsClient});
	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});
	ASSERT_FALSE(_scoreBoard->IsShown());

	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Over});

	EXPECT_TRUE(_scoreBoard->IsShown());
}

// the board is up on the client, then the server announces Playing again: it comes down
TEST_F(ScoreBoardTest, NextMatchHidesScore)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsClient});
	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Over});
	ASSERT_TRUE(_scoreBoard->IsShown());

	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});

	EXPECT_FALSE(_scoreBoard->IsShown());
}

// the attract demo ends the same way, but nobody was playing - no board
TEST_F(ScoreBoardTest, DemoKeepsScoreHidden)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::OnePlayer});
	_events->EmitEvent(DemoStartedEvent{});

	_events->EmitEvent(GameFinishedEvent{.state = GameState::Over});

	EXPECT_FALSE(_scoreBoard->IsShown());
}
