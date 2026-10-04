#include "application/PauseSwitch.h"
#include "components/EventSystem.h"
#include "components/MatchSettings.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/InputEvents.h"
#include "components/managers/GameStateManager.h"
#include "enums/Absence.h"
#include "enums/DisconnectReason.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include "enums/PlayerSlot.h"
#include "gtest/gtest.h"
#include <array>
#include <memory>
#include <optional>
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
	_stateManager = std::make_unique<GameStateManager>(_events, MatchSettings{.seats = 4u});
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

// a host told to start at once does not wait for the seats nobody took
TEST_F(GameStateTest, AMatchStartingAtOnceStartsWithTheFirstReady)
{
	_stateManager = std::make_unique<GameStateManager>(_events, MatchSettings{.seats = 4u, .isStartingAtOnce = true});
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});

	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
}

//NOTE: a client already in would otherwise join its own match running, through a pause and a snapshot
TEST_F(GameStateTest, AMatchStartingAtOnceStillWaitsForAClientAlreadySeated)
{
	_stateManager = std::make_unique<GameStateManager>(_events, MatchSettings{.seats = 4u, .isStartingAtOnce = true});
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerClientSeatedEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerClientSeatedEvent{.slot = PlayerSlot::P2});

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	ASSERT_EQ(GameState::Lobby, _stateManager->GetState()) << "the match started without the seated second client";

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
}

// the seated client that went away was all the match waited for
TEST_F(GameStateTest, ASeatedClientLeavingStartsTheMatchThatWaitedForIt)
{
	_stateManager = std::make_unique<GameStateManager>(_events, MatchSettings{.seats = 4u, .isStartingAtOnce = true});
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerClientSeatedEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	ASSERT_EQ(GameState::Lobby, _stateManager->GetState());

	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P2});

	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
}

// a match waiting for every seat counts the bots in
TEST_F(GameStateTest, TheBotsCountTowardsAFullMatch)
{
	_stateManager = std::make_unique<GameStateManager>(_events, MatchSettings{.seats = 4u, .bots = 2u});
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	ASSERT_EQ(GameState::Lobby, _stateManager->GetState()) << "one player and two bots filled four seats";

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
}

// with no enemies the players have only each other - a match starting at once still waits for a second one
TEST_F(GameStateTest, AMatchWithNoEnemiesWaitsForASecondPlayer)
{
	_stateManager = std::make_unique<GameStateManager>(
			_events, MatchSettings{.seats = 4u, .enemiesAtOnce = 0u, .isStartingAtOnce = true});
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	ASSERT_EQ(GameState::Lobby, _stateManager->GetState()) << "the match started with one player and nobody to fight";

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
}

// a bot in a seat is somebody to fight too
TEST_F(GameStateTest, ABotInASeatIsTheSecondAMatchWithNoEnemiesWaitsFor)
{
	_stateManager = std::make_unique<GameStateManager>(
			_events, MatchSettings{.seats = 2u, .enemiesAtOnce = 0u, .bots = 1u, .isStartingAtOnce = true});
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});

	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
}

//NOTE: the top ones - a player joining later sits in an empty seat before taking a bot's
TEST_F(GameStateTest, TheBotsTakeTheTopSeatsNobodyTook)
{
	std::optional<SeatsFilledEvent> filled{};
	const EventSubscription filledSub{_events->AddListener([&filled](const SeatsFilledEvent& event)
	{
		filled = event;
	})};
	_stateManager = std::make_unique<GameStateManager>(
			_events, MatchSettings{.seats = 4u, .bots = 2u, .isStartingAtOnce = true});
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});

	ASSERT_TRUE(filled.has_value()) << "the match started without saying who sits where";
	EXPECT_EQ(filled->holders, (std::array{SeatHolder::Player, SeatHolder::Empty, SeatHolder::Bot, SeatHolder::Bot}));
}

// a bot fills a seat only when nobody is in it
TEST_F(GameStateTest, NoBotSitsWhereAPlayerIs)
{
	std::optional<SeatsFilledEvent> filled{};
	const EventSubscription filledSub{_events->AddListener([&filled](const SeatsFilledEvent& event)
	{
		filled = event;
	})};
	_stateManager = std::make_unique<GameStateManager>(
			_events, MatchSettings{.seats = 4u, .bots = 3u, .isStartingAtOnce = true});
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerClientSeatedEvent{.slot = PlayerSlot::P4});

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P4});

	ASSERT_TRUE(filled.has_value());
	EXPECT_EQ(filled->holders, (std::array{SeatHolder::Player, SeatHolder::Bot, SeatHolder::Bot, SeatHolder::Player}));
}

// a player joining a running match is put in the seat a bot held, and the bot is told to give it up
TEST_F(GameStateTest, AJoinTakesTheSeatOverFromItsBot)
{
	std::optional<SeatHolderChangedEvent> taken{};
	const EventSubscription takenSub{_events->AddListener([&taken](const SeatHolderChangedEvent& event)
	{
		taken = event;
	})};
	_stateManager = std::make_unique<GameStateManager>(
			_events, MatchSettings{.seats = 2u, .bots = 1u, .isStartingAtOnce = true});
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	ASSERT_EQ(GameState::Playing, _stateManager->GetState());
	ASSERT_FALSE(taken.has_value());
	_matchStarts = 0;

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});

	ASSERT_TRUE(taken.has_value());
	EXPECT_EQ(taken->slot, PlayerSlot::P2);
	EXPECT_EQ(taken->from, SeatHolder::Bot);
	EXPECT_EQ(_matchStarts, 0) << "the join restarted the match";
}

// the match stands still from the join until the newcomer says the field reached it
TEST_F(GameStateTest, AJoinHoldsTheMatchUntilTheFieldReachesTheNewcomer)
{
	const PauseSwitch pauseSwitch{_events};
	std::vector<bool> pauses{};
	const EventSubscription pauseSub{_events->AddListener([&pauses](const PauseStatusEvent& event)
	{
		pauses.push_back(event.isPaused);
	})};
	_stateManager = std::make_unique<GameStateManager>(_events, MatchSettings{.seats = 2u, .isStartingAtOnce = true});
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	ASSERT_EQ(pauses, std::vector{true});

	_events->EmitEvent(ServerClientSyncedEvent{.slot = PlayerSlot::P2});
	EXPECT_EQ(pauses, (std::vector{true, false}));
}

// two newcomers at once - the first one caught up does not let the match go for the other
TEST_F(GameStateTest, TheHoldWaitsForEveryNewcomer)
{
	const PauseSwitch pauseSwitch{_events};
	std::vector<bool> pauses{};
	const EventSubscription pauseSub{_events->AddListener([&pauses](const PauseStatusEvent& event)
	{
		pauses.push_back(event.isPaused);
	})};
	_stateManager = std::make_unique<GameStateManager>(_events, MatchSettings{.seats = 3u, .isStartingAtOnce = true});
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P3});

	_events->EmitEvent(ServerClientSyncedEvent{.slot = PlayerSlot::P2});
	ASSERT_EQ(pauses, std::vector{true}) << "the match went on before the second newcomer had the field";

	_events->EmitEvent(ServerClientSyncedEvent{.slot = PlayerSlot::P3});
	EXPECT_EQ(pauses, (std::vector{true, false}));
}

//NOTE: the newcomer is not caught up yet - a player letting go of the pause is told it still stands
TEST_F(GameStateTest, AnUnpauseDuringTheHoldDoesNotLetTheMatchGo)
{
	const PauseSwitch pauseSwitch{_events};
	std::vector<bool> pauses{};
	const EventSubscription pauseSub{_events->AddListener([&pauses](const PauseStatusEvent& event)
	{
		pauses.push_back(event.isPaused);
	})};
	_stateManager = std::make_unique<GameStateManager>(_events, MatchSettings{.seats = 2u, .isStartingAtOnce = true});
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});

	_events->EmitEvent(SetPauseEvent{.isPaused = false});
	EXPECT_EQ(pauses, (std::vector{true, true})) << "the unpause was not answered with the pause that still stands";

	_events->EmitEvent(ServerClientSyncedEvent{.slot = PlayerSlot::P2});
	EXPECT_EQ(pauses, (std::vector{true, true, false}));
}

//NOTE: a player pausing while the field is on its way meant it - the end of the sync does not lift it
TEST_F(GameStateTest, APauseAskedForDuringTheHoldOutlastsIt)
{
	const PauseSwitch pauseSwitch{_events};
	std::vector<bool> pauses{};
	const EventSubscription pauseSub{_events->AddListener([&pauses](const PauseStatusEvent& event)
	{
		pauses.push_back(event.isPaused);
	})};
	_stateManager = std::make_unique<GameStateManager>(_events, MatchSettings{.seats = 2u, .isStartingAtOnce = true});
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});

	_events->EmitEvent(SetPauseEvent{.isPaused = true});
	_events->EmitEvent(ServerClientSyncedEvent{.slot = PlayerSlot::P2});

	EXPECT_EQ(pauses, std::vector{true});
	EXPECT_EQ(GameState::Paused, _stateManager->GetState());
}

//NOTE: the hold let go of on the way to the lobby must not resume the match being torn down
TEST_F(GameStateTest, ARestartDuringTheHoldGoesStraightToTheLobby)
{
	const PauseSwitch pauseSwitch{_events};
	_stateManager = std::make_unique<GameStateManager>(_events, MatchSettings{.seats = 2u, .isStartingAtOnce = true});
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	ASSERT_EQ(GameState::Paused, _stateManager->GetState());
	_announced.clear();

	_events->EmitEvent(ServerInRestartRequestedEvent{});

	EXPECT_EQ(_announced, std::vector{GameState::Lobby});
}

// a newcomer lost on its way in is asked about like anyone who left, and holds nobody up once answered
TEST_F(GameStateTest, ANewcomerLostBeforeItSyncedLetsTheMatchGoOn)
{
	const PauseSwitch pauseSwitch{_events};
	std::vector<bool> pauses{};
	const EventSubscription pauseSub{_events->AddListener([&pauses](const PauseStatusEvent& event)
	{
		pauses.push_back(event.isPaused);
	})};
	_stateManager = std::make_unique<GameStateManager>(_events, MatchSettings{.seats = 2u, .isStartingAtOnce = true});
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});

	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(AbsenceChosenEvent{.choice = AbsenceChoice::Continue});

	EXPECT_EQ(pauses, (std::vector{true, false}));
	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
}

// a player leaving a running match holds it, and the ones still in are asked about the seat
TEST_F(GameStateTest, APlayerLeavingHoldsTheMatchAndAsksTheRest)
{
	const PauseSwitch pauseSwitch{_events};
	std::optional<AbsenceChangedEvent> asked{};
	const EventSubscription askedSub{_events->AddListener([&asked](const AbsenceChangedEvent& event)
	{
		asked = event;
	})};
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	ASSERT_EQ(GameState::Playing, _stateManager->GetState());

	_events->EmitEvent(ServerInDisconnectEvent{.reason = DisconnectReason::GameOver, .slot = PlayerSlot::P2});

	ASSERT_TRUE(asked.has_value());
	EXPECT_EQ(asked->seats, (std::array{Absence::None, Absence::Left, Absence::None, Absence::None}));
	EXPECT_EQ(GameState::Paused, _stateManager->GetState());
}

//NOTE: the panel holds the match, not the pause key - an unpause while it asks is answered with the pause
TEST_F(GameStateTest, AnUnpauseDoesNotAnswerThePanel)
{
	const PauseSwitch pauseSwitch{_events};
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P2});

	_events->EmitEvent(SetPauseEvent{.isPaused = false});

	EXPECT_EQ(GameState::Paused, _stateManager->GetState());
}

// playing on without the one who left lets the match go, and nobody is asked any more
TEST_F(GameStateTest, PlayingOnWithoutTheOneWhoLeftLetsTheMatchGo)
{
	const PauseSwitch pauseSwitch{_events};
	std::optional<AbsenceChangedEvent> asked{};
	const EventSubscription askedSub{_events->AddListener([&asked](const AbsenceChangedEvent& event)
	{
		asked = event;
	})};
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P2});

	_events->EmitEvent(AbsenceChosenEvent{.choice = AbsenceChoice::Continue});

	ASSERT_TRUE(asked.has_value());
	EXPECT_EQ(asked->seats, (std::array<Absence, kSeatCount>{}));
	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
}

// playing on without the one who left gives the seat up to nobody - kept for the one who left, not a bot's
TEST_F(GameStateTest, PlayingOnWithoutGivesTheSeatUpToNobody)
{
	std::vector<SeatHolderChangedEvent> changed{};
	const EventSubscription changedSub{_events->AddListener([&changed](const SeatHolderChangedEvent& event)
	{
		changed.push_back(event);
	})};
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P2});

	_events->EmitEvent(AbsenceChosenEvent{.choice = AbsenceChoice::Continue});

	ASSERT_EQ(changed.size(), 1u);
	EXPECT_EQ(changed.front().slot, PlayerSlot::P2);
	EXPECT_EQ(changed.front().from, SeatHolder::Player);
	EXPECT_EQ(changed.front().to, SeatHolder::Empty);
}

//NOTE: the one who comes back is shown, but the match goes on only once somebody says so - even after the sync
TEST_F(GameStateTest, TheOneBackIsShownAndTheMatchWaitsForAnAnswer)
{
	const PauseSwitch pauseSwitch{_events};
	std::optional<AbsenceChangedEvent> asked{};
	const EventSubscription askedSub{_events->AddListener([&asked](const AbsenceChangedEvent& event)
	{
		asked = event;
	})};
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P2});

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(ServerClientSyncedEvent{.slot = PlayerSlot::P2});

	ASSERT_TRUE(asked.has_value());
	EXPECT_EQ(asked->seats, (std::array{Absence::None, Absence::Back, Absence::None, Absence::None}));
	ASSERT_EQ(GameState::Paused, _stateManager->GetState()) << "the match went on before anyone said so";

	_events->EmitEvent(AbsenceChosenEvent{.choice = AbsenceChoice::Continue});
	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
}

// a bot takes over the seat of the one who left, and gives it back when the player returns
TEST_F(GameStateTest, ABotTakesOverTheSeatOfTheOneWhoLeftUntilItsPlayerIsBack)
{
	const PauseSwitch pauseSwitch{_events};
	std::vector<SeatHolderChangedEvent> taken{};
	const EventSubscription takenSub{_events->AddListener([&taken](const SeatHolderChangedEvent& event)
	{
		taken.push_back(event);
	})};
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P2});

	_events->EmitEvent(AbsenceChosenEvent{.choice = AbsenceChoice::Bot});

	ASSERT_EQ(taken.size(), 1u);
	EXPECT_EQ(taken.front().slot, PlayerSlot::P2);
	EXPECT_EQ(taken.front().from, SeatHolder::Player);
	EXPECT_EQ(taken.front().to, SeatHolder::Bot);
	ASSERT_EQ(GameState::Playing, _stateManager->GetState());

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});

	ASSERT_EQ(taken.size(), 2u);
	EXPECT_EQ(taken.back().from, SeatHolder::Bot);
	EXPECT_EQ(taken.back().to, SeatHolder::Player);
}

// the first answer decides - a second one, sent before the first was heard, finds nobody asked about
TEST_F(GameStateTest, OnlyTheFirstAnswerCounts)
{
	bool isBotSeated{};
	const EventSubscription changedSub{_events->AddListener([&isBotSeated](const SeatHolderChangedEvent& event)
	{
		isBotSeated = isBotSeated || event.to == SeatHolder::Bot;
	})};
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P2});

	_events->EmitEvent(AbsenceChosenEvent{.choice = AbsenceChoice::Continue});
	_events->EmitEvent(AbsenceChosenEvent{.choice = AbsenceChoice::Bot});

	EXPECT_FALSE(isBotSeated) << "a late answer handed the seat to a bot after the match went on without it";
}

// a match already over has nothing to hold - the scoreboard stands for everyone
TEST_F(GameStateTest, ALeaveOnTheScoreboardAsksNothing)
{
	bool isAsked{};
	const EventSubscription askedSub{_events->AddListener([&isAsked](const AbsenceChangedEvent&) { isAsked = true; })};
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(GameFinishedEvent{.state = GameState::Won});

	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P2});

	EXPECT_FALSE(isAsked);
}

//NOTE: nobody left to answer - the match goes to the lobby, and the hold let go of on the way does not resume it
TEST_F(GameStateTest, TheLastOneLeavingTakesTheHeldMatchToTheLobby)
{
	const PauseSwitch pauseSwitch{_events};
	std::optional<AbsenceChangedEvent> asked{};
	const EventSubscription askedSub{_events->AddListener([&asked](const AbsenceChangedEvent& event)
	{
		asked = event;
	})};
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P2});
	_announced.clear();

	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P1});

	EXPECT_EQ(_announced, std::vector{GameState::Lobby});
	ASSERT_TRUE(asked.has_value());
	EXPECT_EQ(asked->seats, (std::array<Absence, kSeatCount>{})) << "the clients are still asked about a match gone";
}

//NOTE: a client only mirrors who left - its own answer goes to the server, and nothing changes here until it answers
TEST_F(GameStateTest, AClientsAnswerChangesNothingOnTheClient)
{
	bool isSeatTaken{};
	const EventSubscription takenSub{_events->AddListener([&isSeatTaken](const SeatHolderChangedEvent&)
	{
		isSeatTaken = true;
	})};
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsClient});
	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Paused});
	_events->EmitEvent(AbsenceChangedEvent{.seats = {Absence::None, Absence::Left, Absence::None, Absence::None}});

	_events->EmitEvent(AbsenceChosenEvent{.choice = AbsenceChoice::Bot});

	EXPECT_FALSE(isSeatTaken) << "the client handed a seat to a bot on its own";
}

//NOTE: a restart drops who left with the match - the next one would start held for a player nobody waits for
TEST_F(GameStateTest, ARestartForgetsWhoLeft)
{
	const PauseSwitch pauseSwitch{_events};
	bool isPaused{};
	const EventSubscription pauseSub{_events->AddListener([&isPaused](const PauseStatusEvent& event)
	{
		isPaused = event.isPaused;
	})};
	_stateManager = std::make_unique<GameStateManager>(_events, MatchSettings{.seats = 3u});
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	for (const PlayerSlot slot: {PlayerSlot::P1, PlayerSlot::P2, PlayerSlot::P3})
	{
		_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = slot});
	}
	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P3});
	ASSERT_TRUE(isPaused);

	_events->EmitEvent(ServerInRestartRequestedEvent{});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P3});

	EXPECT_EQ(GameState::Playing, _stateManager->GetState());
	EXPECT_FALSE(isPaused) << "the new match is held for the one who left the old one";
}
