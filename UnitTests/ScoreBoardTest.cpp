#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/GameStatistics.h"
#include "components/ScoreBoard.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/RenderUIEvents.h"
#include "components/managers/GameStateManager.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include "gtest/gtest.h"
#include <memory>

class ScoreBoardTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<GameStatistics> _statistics{nullptr};
	std::unique_ptr<GameStateManager> _stateManager{nullptr};
	std::unique_ptr<ScoreBoard> _scoreBoard{nullptr};
	EventSubscription _showedSub{};
	GameConfig _gameConfig{};
	bool _isScoreShowed{false};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_statistics = std::make_unique<GameStatistics>(_events);
		_stateManager = std::make_unique<GameStateManager>(_events);
		_scoreBoard = std::make_unique<ScoreBoard>(_events, _gameConfig, *_statistics);

		_showedSub = _events->AddListener([this](const ScoreBoardShowedEvent& event)
		{
			_isScoreShowed = event.isDisplayed;
		});
	}
};

TEST_F(ScoreBoardTest, GameOverShowsScore)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::OnePlayer});
	ASSERT_FALSE(_isScoreShowed);

	_events->EmitEvent(GameFinishedEvent{.state = GameState::Over});

	EXPECT_TRUE(_isScoreShowed);
}

TEST_F(ScoreBoardTest, GameWonShowsScore)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::OnePlayer});
	_events->EmitEvent(GameFinishedEvent{.state = GameState::Won});

	EXPECT_TRUE(_isScoreShowed);
}

//NOTE: a client never runs the win check, so the phase arriving from the host is all it has to go on
TEST_F(ScoreBoardTest, HostAnnouncedGameOverShowsScoreOnClient)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsClient});
	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});
	ASSERT_FALSE(_isScoreShowed);

	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Over});

	EXPECT_TRUE(_isScoreShowed);
}

TEST_F(ScoreBoardTest, NextMatchHidesScore)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsClient});
	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Over});
	ASSERT_TRUE(_isScoreShowed);

	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});

	EXPECT_FALSE(_isScoreShowed);
}

TEST_F(ScoreBoardTest, DemoKeepsScoreHidden)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::OnePlayer});
	_events->EmitEvent(DemoStartedEvent{});

	_events->EmitEvent(GameFinishedEvent{.state = GameState::Over});

	EXPECT_FALSE(_isScoreShowed);
}
