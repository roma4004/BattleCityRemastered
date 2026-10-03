#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/InputEvents.h"
#include "components/managers/GameStateManager.h"
#include "enums/DisconnectReason.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include "enums/PlayerSlot.h"
#include "gtest/gtest.h"
#include <memory>
#include <vector>

// the phase machine alone: a mode is applied, events are fed in, and the phase, the announcements or the
// match starts are read back
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

// a four-seat host waits for four readies
TEST_F(GameStateTest, AFourSeatServerWaitsForEveryReady)
{
	_stateManager = std::make_unique<GameStateManager>(_events, 4u);
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});

	for (const PlayerSlot slot: {PlayerSlot::P1, PlayerSlot::P2, PlayerSlot::P3})
	{
		_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = slot});
	}

	ASSERT_EQ(GameState::Lobby, _stateManager->GetState()) << "three of four started the match";

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P4});

	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
}

// a local mode has both seats filled from the start, so the match begins on the spot
TEST_F(GameStateTest, LocalGameStartsPlayingWithNoOneToWaitFor)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::OnePlayer});

	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
	EXPECT_EQ(std::vector{GameState::Playing}, _announced);
}

// a network mode has nobody in its seats yet and waits
TEST_F(GameStateTest, NetworkGameStartsInTheLobby)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});

	EXPECT_EQ(GameState::Lobby, _stateManager->GetState());
}

// one ready seat is not enough for the server - the second one starts the match
TEST_F(GameStateTest, TheServerLeavesTheLobbyOnceBothSeatsAreReady)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	EXPECT_EQ(GameState::Lobby, _stateManager->GetState()) << "the first player alone started the match";

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
}

//NOTE: the link says nothing about the other seat - starting on it played alone on an empty field
TEST_F(GameStateTest, AClientStaysInTheLobbyUntilTheServerSaysOtherwise)
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

//NOTE: the one who stayed keeps playing - the seat is taken back later with a snapshot of the field
TEST_F(GameStateTest, ALostSeatLeavesTheMatchToThePlayerWhoStayed)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	ASSERT_EQ(GameState::Playing, _stateManager->GetState());
	_matchStarts = 0;

	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P2});
	EXPECT_EQ(GameState::Playing, _stateManager->GetState());

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
	EXPECT_EQ(_matchStarts, 0) << "the returning player restarted the match of the one who stayed";
}

// a quit and a lost link, in both orders: whichever empties the last seat ends the match
TEST_F(GameStateTest, EveryKindOfLeaveEmptyingTheLastSeatGoesBackToTheLobby)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(ServerInDisconnectEvent{.reason = DisconnectReason::PlayerQuit, .slot = PlayerSlot::P1});
	ASSERT_EQ(GameState::Playing, _stateManager->GetState());

	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P2});
	EXPECT_EQ(GameState::Lobby, _stateManager->GetState());

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P1});
	ASSERT_EQ(GameState::Playing, _stateManager->GetState());

	_events->EmitEvent(ServerInDisconnectEvent{.reason = DisconnectReason::PlayerQuit, .slot = PlayerSlot::P2});
	EXPECT_EQ(GameState::Lobby, _stateManager->GetState());
}

// and on a client, every way of losing the host does the same
TEST_F(GameStateTest, EveryKindOfHostLossSendsAClientBackToTheLobby)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsClient});

	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});
	ASSERT_EQ(GameState::Playing, _stateManager->GetState());
	_events->EmitEvent(ClientInDisconnectEvent{.reason = DisconnectReason::HostShutdown});
	EXPECT_EQ(GameState::Lobby, _stateManager->GetState());

	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});
	ASSERT_EQ(GameState::Playing, _stateManager->GetState());
	_events->EmitEvent(ClientReconnectAbandonedEvent{});
	EXPECT_EQ(GameState::Lobby, _stateManager->GetState());

	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});
	ASSERT_EQ(GameState::Playing, _stateManager->GetState());
	_events->EmitEvent(ClientHostLostEvent{});
	EXPECT_EQ(GameState::Lobby, _stateManager->GetState());
}

// a client that gave up reconnecting dials again and waits for the phase once more
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

// a network event reaching a local match is ignored - there is no lobby to go back to
TEST_F(GameStateTest, ALocalGameNeverEntersTheLobby)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::OnePlayer});
	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P1});

	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
}

// pausing and resuming a running match
TEST_F(GameStateTest, PauseOnlyTogglesWhileTheMatchRuns)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::OnePlayer});

	_events->EmitEvent(PauseStatusEvent{.isPaused = true});
	EXPECT_EQ(GameState::Paused, _stateManager->GetState());

	_events->EmitEvent(PauseStatusEvent{.isPaused = false});
	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
}

// the same key in the lobby changes nothing
TEST_F(GameStateTest, PausingInTheLobbyKeepsTheLobby)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});

	_events->EmitEvent(PauseStatusEvent{.isPaused = true});

	EXPECT_EQ(GameState::Lobby, _stateManager->GetState());
}

// a result stands until the field is cleared, and a pause key does not clear it
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
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
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

// applying the mode again drops the ready seats and puts the server back in the lobby
TEST_F(GameStateTest, LeavingANetworkGameForgetsThePeers)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	ASSERT_EQ(GameState::Playing, _stateManager->GetState());

	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});

	EXPECT_EQ(GameState::Lobby, _stateManager->GetState()) << "a new match kept the peers of the old one";
}

// leaving a pause announces the phase, but it is the same match
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

// a local mode is one announcement and one match start
TEST_F(GameStateTest, ApplyingALocalModeStartsAMatch)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::OnePlayer});

	ASSERT_EQ(_announced.size(), 1u);
	EXPECT_EQ(_announced.front(), GameState::Playing);
	EXPECT_EQ(_matchStarts, 1);
}

// on a server it is the seat that fills last that starts it
TEST_F(GameStateTest, TheLastPeerToJoinStartsTheMatch)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_announced.clear();
	_matchStarts = 0;

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});

	ASSERT_EQ(_announced.size(), 1u);
	EXPECT_EQ(_announced.front(), GameState::Playing);
	EXPECT_EQ(_matchStarts, 1);
}

// Seats, not a count: a client that readies twice fills one seat, not both
TEST_F(GameStateTest, ASeatReadiedTwiceStillWaitsForTheOther)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});

	EXPECT_EQ(GameState::Lobby, _stateManager->GetState());
}

//NOTE: a client dropping before its ready gave back a seat it never took - the one who stayed kept playing
TEST_F(GameStateTest, ALossBeforeTheReadyTakesNoSeatAway)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P2});
	ASSERT_EQ(GameState::Playing, _stateManager->GetState());

	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P2});

	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
}

// The other side of the same counter - a client waits for the server, and the server is one peer
TEST_F(GameStateTest, AClientStartsTheMatchOnlyWhenTheServerAnnouncesIt)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsClient});
	_matchStarts = 0;

	_events->EmitEvent(ClientConnectedToHostEvent{});
	EXPECT_EQ(_matchStarts, 0) << "the link alone started the match";

	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});
	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
	EXPECT_EQ(_matchStarts, 1);
}

// a restart empties both seats, so one ready player waits again
TEST_F(GameStateTest, ARestartPutsTheServerBackToWaitingForBothSeats)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	ASSERT_EQ(GameState::Playing, _stateManager->GetState());

	_events->EmitEvent(ServerInRestartRequestedEvent{});
	EXPECT_EQ(GameState::Lobby, _stateManager->GetState());

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	EXPECT_EQ(GameState::Lobby, _stateManager->GetState()) << "one player restarted the match alone";

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
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

// entering the lobby starts nothing by itself
TEST_F(GameStateTest, WaitingInTheLobbyStartsNoMatch)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});

	EXPECT_EQ(_matchStarts, 0);
}

//NOTE: the menu, not the mode's idle phase - a lobby would start the same match on the same map again
TEST_F(GameStateTest, AMapThatFailsToLoadTakesThePhaseBackToTheMenu)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::OnePlayer});
	ASSERT_EQ(GameState::Playing, _stateManager->GetState());

	_events->EmitEvent(MapLoadFailedEvent{});

	EXPECT_EQ(GameState::Menu, _stateManager->GetState());
	EXPECT_EQ(GameState::Menu, _announced.back());
}

// the host lifts a pause and says Playing again: the client resumes where it stood. A match start would
// empty the field and load the map over it, and the host would not respawn the tanks it still counts alive
TEST_F(GameStateTest, AHostLeavingAPauseStartsNoMatch)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsClient});
	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});
	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Paused});
	_announced.clear();
	_matchStarts = 0;

	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});

	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
	EXPECT_EQ(std::vector{GameState::Playing}, _announced);
	EXPECT_EQ(_matchStarts, 0);
}
