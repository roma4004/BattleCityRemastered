#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/InputEvents.h"
#include "components/managers/GameStateManager.h"
#include "enums/DisconnectReason.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include "gtest/gtest.h"
#include <memory>
#include <vector>

class GameStateTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{std::make_shared<EventSystem>()};
	std::unique_ptr<GameStateManager> _stateManager{nullptr};
	std::vector<GameState> _announced{};
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
		});
		_matchStartSub = _events->AddListener([this](const MatchStartedEvent&)
		{
			++_matchStarts;
			_phasesBeforeMatchStart = _announced.size();
		});
	}
};

TEST_F(GameStateTest, LocalGameStartsPlayingWithNoOneToWaitFor)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::OnePlayer});

	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
	EXPECT_EQ(std::vector{GameState::Playing}, _announced);
}

TEST_F(GameStateTest, NetworkGameStartsInTheLobby)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});

	EXPECT_EQ(GameState::Lobby, _stateManager->GetState());
}

TEST_F(GameStateTest, HostLeavesTheLobbyOnceBothSeatsAreReady)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{});
	EXPECT_EQ(GameState::Lobby, _stateManager->GetState()) << "the first player alone started the match";

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{});
	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
}

//NOTE: the link says nothing about the other seat - starting on it played alone on an empty field
TEST_F(GameStateTest, AClientStaysInTheLobbyUntilTheHostSaysOtherwise)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsClient});
	_events->EmitEvent(ClientConnectedToHostEvent{});
	EXPECT_EQ(GameState::Lobby, _stateManager->GetState());

	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});
	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
}

//NOTE: entering Playing wipes the world on a client, so a spawn burst ahead of it is erased there
TEST_F(GameStateTest, ThePhaseIsAnnouncedBeforeTheMatchStarts)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::OnePlayer});

	ASSERT_EQ(_matchStarts, 1);
	EXPECT_EQ(_phasesBeforeMatchStart, 1u) << "the match started before its phase was announced";
}

TEST_F(GameStateTest, EveryKindOfPeerLossGoesBackToTheLobby)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{});
	ASSERT_EQ(GameState::Playing, _stateManager->GetState());
	_events->EmitEvent(ServerInDisconnectEvent{.reason = DisconnectReason::PlayerQuit});
	EXPECT_EQ(GameState::Lobby, _stateManager->GetState());

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{});
	ASSERT_EQ(GameState::Playing, _stateManager->GetState());
	_events->EmitEvent(ServerClientLostEvent{});
	EXPECT_EQ(GameState::Lobby, _stateManager->GetState());

	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsClient});

	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});
	ASSERT_EQ(GameState::Playing, _stateManager->GetState());
	_events->EmitEvent(ClientInDisconnectEvent{.reason = DisconnectReason::HostShutdown});
	EXPECT_EQ(GameState::Lobby, _stateManager->GetState());

	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});
	ASSERT_EQ(GameState::Playing, _stateManager->GetState());
	_events->EmitEvent(ClientReconnectAbandonedEvent{});
	EXPECT_EQ(GameState::Lobby, _stateManager->GetState());
}

TEST_F(GameStateTest, ReconnectingAfterALossStartsTheMatchAgain)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsClient});
	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});
	_events->EmitEvent(ClientReconnectAbandonedEvent{});
	ASSERT_EQ(GameState::Lobby, _stateManager->GetState());

	_announced.clear();
	_events->EmitEvent(ClientConnectedToHostEvent{});
	EXPECT_EQ(GameState::Lobby, _stateManager->GetState()) << "the link alone started a match";

	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});
	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
	EXPECT_EQ(std::vector{GameState::Playing}, _announced);
}

TEST_F(GameStateTest, ALocalGameNeverEntersTheLobby)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::OnePlayer});
	_events->EmitEvent(ServerClientLostEvent{});

	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
}

TEST_F(GameStateTest, PauseOnlyTogglesWhileTheMatchRuns)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::OnePlayer});

	_events->EmitEvent(PauseStatusEvent{.isPaused = true});
	EXPECT_EQ(GameState::Paused, _stateManager->GetState());

	_events->EmitEvent(PauseStatusEvent{.isPaused = false});
	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
}

TEST_F(GameStateTest, PausingInTheLobbyKeepsTheLobby)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});

	_events->EmitEvent(PauseStatusEvent{.isPaused = true});

	EXPECT_EQ(GameState::Lobby, _stateManager->GetState());
}

TEST_F(GameStateTest, WinAndLossSurviveUntilTheFieldIsCleared)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::OnePlayer});

	_events->EmitEvent(GameFinishedEvent{.state = GameState::Won});
	EXPECT_EQ(GameState::Won, _stateManager->GetState());
	_events->EmitEvent(PauseStatusEvent{.isPaused = true});
	EXPECT_EQ(GameState::Won, _stateManager->GetState()) << "a pause key press erased the result";

	_events->EmitEvent(GameResetEvent{});
	EXPECT_EQ(GameState::Playing, _stateManager->GetState());

	_events->EmitEvent(GameFinishedEvent{.state = GameState::Over});
	EXPECT_EQ(GameState::Over, _stateManager->GetState());
}

//NOTE: entering a phase clears the field, so answering the reset with a phase change would loop
TEST_F(GameStateTest, AResetDuringAMatchChangesNothing)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{});
	ASSERT_EQ(GameState::Playing, _stateManager->GetState());

	_announced.clear();
	_events->EmitEvent(GameResetEvent{});

	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
	EXPECT_TRUE(_announced.empty());
}

//NOTE: same phase name, new match - the entry still has to be announced
TEST_F(GameStateTest, RestartingTheSameModeAnnouncesThePhaseAgain)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::OnePlayer});
	_announced.clear();

	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::OnePlayer});

	EXPECT_EQ(std::vector{GameState::Playing}, _announced);
}

TEST_F(GameStateTest, LeavingANetworkGameForgetsThePeers)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{});
	ASSERT_EQ(GameState::Playing, _stateManager->GetState());

	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});

	EXPECT_EQ(GameState::Lobby, _stateManager->GetState()) << "a new match kept the peers of the old one";
}

TEST_F(GameStateTest, ResumingFromAPauseStartsNoMatch)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::OnePlayer});
	_events->EmitEvent(PauseStatusEvent{.isPaused = true});
	_announced.clear();
	_matchStarts = 0;

	_events->EmitEvent(PauseStatusEvent{.isPaused = false});

	ASSERT_EQ(_announced.size(), 1u);
	EXPECT_EQ(_announced.front(), GameState::Playing);
	EXPECT_EQ(_matchStarts, 0);
}

TEST_F(GameStateTest, ApplyingALocalModeStartsAMatch)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::OnePlayer});

	ASSERT_EQ(_announced.size(), 1u);
	EXPECT_EQ(_announced.front(), GameState::Playing);
	EXPECT_EQ(_matchStarts, 1);
}

TEST_F(GameStateTest, TheLastPeerToJoinStartsTheMatch)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{});
	_announced.clear();
	_matchStarts = 0;

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{});

	ASSERT_EQ(_announced.size(), 1u);
	EXPECT_EQ(_announced.front(), GameState::Playing);
	EXPECT_EQ(_matchStarts, 1);
}

// A count, not a flag: the seat the player who stayed still holds must not be counted twice when
// the one who left is replaced
TEST_F(GameStateTest, AHostThatLostOnePlayerRestartsOnOneArrival)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{});
	_events->EmitEvent(ServerClientLostEvent{});
	ASSERT_EQ(GameState::Lobby, _stateManager->GetState());

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{});

	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
}

// The other side of the same counter - a client waits for the host, and the host is one peer
TEST_F(GameStateTest, AClientStartsTheMatchOnlyWhenTheHostAnnouncesIt)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsClient});
	_matchStarts = 0;

	_events->EmitEvent(ClientConnectedToHostEvent{});
	EXPECT_EQ(_matchStarts, 0) << "the link alone started the match";

	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});
	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
	EXPECT_EQ(_matchStarts, 1);
}

TEST_F(GameStateTest, ARestartPutsTheHostBackToWaitingForBothSeats)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{});
	ASSERT_EQ(GameState::Playing, _stateManager->GetState());

	_events->EmitEvent(ServerInRestartRequestedEvent{});
	EXPECT_EQ(GameState::Lobby, _stateManager->GetState());

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{});
	EXPECT_EQ(GameState::Lobby, _stateManager->GetState()) << "one player restarted the match alone";

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{});
	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
}

//NOTE: hearing the lobby is what makes a client drop its world, so a repeat still has to go out
TEST_F(GameStateTest, ARestartAnnouncesTheLobbyItIsAlreadyIn)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	ASSERT_EQ(GameState::Lobby, _stateManager->GetState());

	_announced.clear();
	_events->EmitEvent(ServerInRestartRequestedEvent{});

	EXPECT_EQ(std::vector{GameState::Lobby}, _announced);
}

TEST_F(GameStateTest, WaitingInTheLobbyStartsNoMatch)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});

	EXPECT_EQ(_matchStarts, 0);
}
