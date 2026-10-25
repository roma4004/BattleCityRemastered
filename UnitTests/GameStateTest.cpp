#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/InputEvents.h"
#include "components/input/InputProviderForMenu.h"
#include "components/managers/GameStateManager.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include "enums/PlayerSlot.h"
#include "gtest/gtest.h"
#include <cstddef>
#include <memory>
#include <vector>

// the phase machine alone: apply a mode, feed events, read back the phase, the announcements or the match starts
class GameStateTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{std::make_shared<EventSystem>()};
	std::unique_ptr<GameStateManager> _stateManager{nullptr};
	std::vector<GameState> _announced{};
	GameState _phase{};
	int _matchStarts{};
	size_t _phasesBeforeMatchStart{};
	EventSubscription _stateSub{};
	EventSubscription _matchStartSub{};

	void SetUp() override
	{
		_stateManager = std::make_unique<GameStateManager>(_events);
		_stateSub = _events->AddListener([this](const GameStateChangedToEvent& event)
		{
			_announced.push_back(event.state);
			_phase = event.state;
		});
		_matchStartSub = _events->AddListener([this](const MatchStartedEvent&)
		{
			++_matchStarts;
			_phasesBeforeMatchStart = _announced.size();
		});
	}
};

// a local match starts on the spot, its phase announced first, and no network event sends it to a lobby
TEST_F(GameStateTest, ALocalMatchStartsOnTheSpotAndNeverEntersTheLobby)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::OnePlayer});

	EXPECT_EQ(GameState::Playing, _phase);
	EXPECT_EQ(std::vector{GameState::Playing}, _announced);
	ASSERT_EQ(_matchStarts, 1);
	EXPECT_EQ(_phasesBeforeMatchStart, 1u) << "the match started before its phase was announced";

	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P1});

	EXPECT_EQ(GameState::Playing, _phase);
}

// leaving a pause announces the phase, but it is the same match
TEST_F(GameStateTest, ResumingFromAPauseStartsNoMatch)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::OnePlayer});
	_events->EmitEvent(PauseStatusEvent{.isPaused = true});
	_announced.clear();
	_matchStarts = 0;

	_events->EmitEvent(PauseStatusEvent{.isPaused = false});

	EXPECT_EQ(GameState::Playing, _phase);
	EXPECT_EQ(std::vector{GameState::Playing}, _announced);
	EXPECT_EQ(_matchStarts, 0);
}

// clearing the field is what ends a result - the next match is played afresh
TEST_F(GameStateTest, AResetClearsTheResult)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::OnePlayer});
	_events->EmitEvent(GameFinishedEvent{.state = GameState::Over});
	ASSERT_EQ(GameState::Over, _phase);

	_events->EmitEvent(GameResetEvent{});

	EXPECT_EQ(GameState::Playing, _phase);
}

// a pause from outside - the host's or a hold's - is lifted by a reset, by the pause key, and by one release
TEST_F(GameStateTest, APauseFromOutsideIsLiftedByAResetThePauseKeyOrOneRelease)
{
	const GameConfig gameConfig{};
	const InputProviderForMenu menuInput{_events, gameConfig};
	bool isPaused{};
	const EventSubscription statusSub{_events->AddListener([&isPaused](const PauseStatusEvent& event)
	{
		isPaused = event.isPaused;
	})};

	_events->EmitEvent(SetPauseEvent{.isPaused = true});
	ASSERT_TRUE(isPaused);
	_events->EmitEvent(GameResetEvent{});
	EXPECT_FALSE(isPaused) << "a fresh match started frozen";

	_events->EmitEvent(SetPauseEvent{.isPaused = true});
	_events->EmitEvent(PauseReleasedEvent{});
	EXPECT_FALSE(isPaused) << "the pause key stacked a second pause nobody can leave";

	_events->EmitEvent(SetPauseEvent{.isPaused = true});
	_events->EmitEvent(SetPauseEvent{.isPaused = true});
	_events->EmitEvent(SetPauseEvent{.isPaused = false});
	EXPECT_FALSE(isPaused) << "a repeated request needed a second release";
}

//NOTE: the menu, not the mode's idle phase - a lobby would start the same match on the same map again
TEST_F(GameStateTest, AMapThatFailsToLoadTakesThePhaseBackToTheMenu)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::OnePlayer});
	ASSERT_EQ(GameState::Playing, _phase);

	_events->EmitEvent(MapLoadFailedEvent{});

	EXPECT_EQ(GameState::Menu, _phase);
	EXPECT_EQ(GameState::Menu, _announced.back());
}
