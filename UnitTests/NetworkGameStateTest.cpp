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
#include <string_view>
#include <vector>

// the phase machine alone: apply a mode, feed events, read back the phase, the announcements or the match starts
class NetworkGameStateTest : public testing::Test
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

// the server waits in the lobby for the seats it needs, and the ready that completes them starts the match
TEST_F(NetworkGameStateTest, TheReadyThatCompletesTheSeatsStartsTheMatch)
{
	struct Start final
	{
		std::string_view why;
		MatchSettings match{};
		std::vector<PlayerSlot> seated{};
		std::vector<PlayerSlot> early{};
		PlayerSlot last{};
	};

	const std::array starts{
			Start{.why = "two seats", .match = {.seats = 2u}, .early = {PlayerSlot::P1}, .last = PlayerSlot::P2},
			Start{.why = "a seat readied twice is one seat",
				  .match = {.seats = 2u},
				  .early = {PlayerSlot::P1, PlayerSlot::P1},
				  .last = PlayerSlot::P2},
			Start{.why = "four seats",
				  .match = {.seats = 4u},
				  .early = {PlayerSlot::P1, PlayerSlot::P2, PlayerSlot::P3},
				  .last = PlayerSlot::P4},
			Start{.why = "the bots count towards a full match",
				  .match = {.seats = 4u, .bots = 2u},
				  .early = {PlayerSlot::P1},
				  .last = PlayerSlot::P2},
			Start{.why = "starting at once", .match = {.seats = 4u, .isStartingAtOnce = true}, .last = PlayerSlot::P1},
			Start{.why = "starting at once still waits for a client already seated",
				  .match = {.seats = 4u, .isStartingAtOnce = true},
				  .seated = {PlayerSlot::P1, PlayerSlot::P2},
				  .early = {PlayerSlot::P1},
				  .last = PlayerSlot::P2},
			Start{.why = "no enemies - somebody to fight is a second player",
				  .match = {.seats = 4u, .enemiesAtOnce = 0u, .isStartingAtOnce = true},
				  .early = {PlayerSlot::P1},
				  .last = PlayerSlot::P2},
			Start{.why = "no enemies - a bot in a seat is the second",
				  .match = {.seats = 2u, .enemiesAtOnce = 0u, .bots = 1u, .isStartingAtOnce = true},
				  .last = PlayerSlot::P1}};
	for (const Start& start: starts)
	{
		SCOPED_TRACE(start.why);
		_stateManager = std::make_unique<GameStateManager>(_events, start.match);
		_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
		for (const PlayerSlot slot: start.seated)
		{
			_events->EmitEvent(ServerClientSeatedEvent{.slot = slot});
		}
		for (const PlayerSlot slot: start.early)
		{
			_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = slot});
		}
		ASSERT_EQ(GameState::Lobby, _phase) << "the match started a seat short";
		ASSERT_EQ(_matchStarts, 0);
		_announced.clear();

		_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = start.last});

		EXPECT_EQ(GameState::Playing, _phase);
		EXPECT_EQ(std::vector{GameState::Playing}, _announced);
		EXPECT_EQ(_matchStarts, 1);
		_matchStarts = 0;
	}
}

//NOTE: the link says nothing about the other seat - starting on it played alone on an empty field
TEST_F(NetworkGameStateTest, AClientStartsTheMatchOnlyWhenTheServerAnnouncesIt)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsClient});
	_matchStarts = 0;

	_events->EmitEvent(ClientConnectedToHostEvent{});
	EXPECT_EQ(GameState::Lobby, _phase);
	EXPECT_EQ(_matchStarts, 0) << "the link alone started the match";

	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});
	EXPECT_EQ(GameState::Playing, _phase);
	EXPECT_EQ(_matchStarts, 1);

	_events->EmitEvent(ClientReconnectAbandonedEvent{});
	ASSERT_EQ(GameState::Lobby, _phase);
	_announced.clear();
	_events->EmitEvent(ClientConnectedToHostEvent{});
	EXPECT_EQ(GameState::Lobby, _phase) << "dialling again alone started a match";

	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});
	EXPECT_EQ(GameState::Playing, _phase);
	EXPECT_EQ(std::vector{GameState::Playing}, _announced);
}

//NOTE: the seat is taken back later with a snapshot of the field, not with a match of its own
TEST_F(NetworkGameStateTest, APlayerBackInALostSeatDoesNotRestartTheMatch)
{
	const PauseSwitch pauseSwitch{_events};
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	ASSERT_EQ(GameState::Playing, _phase);
	_matchStarts = 0;

	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});

	EXPECT_EQ(_matchStarts, 0) << "the returning player restarted the match of the one who stayed";
}

// a quit and a lost link, in both orders: whichever empties the last seat ends the match
TEST_F(NetworkGameStateTest, EveryKindOfLeaveEmptyingTheLastSeatGoesBackToTheLobby)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(ServerInDisconnectEvent{.reason = DisconnectReason::PlayerQuit, .slot = PlayerSlot::P1});
	ASSERT_EQ(GameState::Playing, _phase);

	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P2});
	EXPECT_EQ(GameState::Lobby, _phase);

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P1});
	ASSERT_EQ(GameState::Playing, _phase);

	_events->EmitEvent(ServerInDisconnectEvent{.reason = DisconnectReason::PlayerQuit, .slot = PlayerSlot::P2});
	EXPECT_EQ(GameState::Lobby, _phase);
}

// and on a client, every way of losing the host does the same
TEST_F(NetworkGameStateTest, EveryKindOfHostLossSendsAClientBackToTheLobby)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsClient});

	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});
	ASSERT_EQ(GameState::Playing, _phase);
	_events->EmitEvent(ClientInDisconnectEvent{.reason = DisconnectReason::HostShutdown});
	EXPECT_EQ(GameState::Lobby, _phase);

	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});
	ASSERT_EQ(GameState::Playing, _phase);
	_events->EmitEvent(ClientReconnectAbandonedEvent{});
	EXPECT_EQ(GameState::Lobby, _phase);

	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});
	ASSERT_EQ(GameState::Playing, _phase);
	_events->EmitEvent(ClientHostLostEvent{});
	EXPECT_EQ(GameState::Lobby, _phase);
}

// the pause key pauses only a running match - the lobby stays the lobby, a result stands
TEST_F(NetworkGameStateTest, ThePauseKeyPausesOnlyARunningMatch)
{
	struct Pause final
	{
		std::string_view why;
		GameMode mode;
		std::optional<GameState> result;
		GameState paused;
	};

	constexpr std::array kPauses{
			Pause{.why = "a running match", .mode = GameMode::OnePlayer, .result = {}, .paused = GameState::Paused},
			Pause{.why = "the lobby", .mode = GameMode::PlayAsHost, .result = {}, .paused = GameState::Lobby},
			Pause{.why = "a result", .mode = GameMode::OnePlayer, .result = GameState::Won, .paused = GameState::Won}};
	for (const Pause& pause: kPauses)
	{
		SCOPED_TRACE(pause.why);
		_stateManager = std::make_unique<GameStateManager>(_events);
		_events->EmitEvent(GameModeAppliedEvent{.mode = pause.mode});
		if (pause.result.has_value())
		{
			_events->EmitEvent(GameFinishedEvent{.state = *pause.result});
		}

		_events->EmitEvent(PauseStatusEvent{.isPaused = true});

		EXPECT_EQ(pause.paused, _phase);
	}
}

//NOTE: entering a phase clears the field, so answering the reset with a phase change would loop
TEST_F(NetworkGameStateTest, AResetDuringAMatchChangesNothing)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	ASSERT_EQ(GameState::Playing, _phase);

	_announced.clear();
	_events->EmitEvent(GameResetEvent{});

	EXPECT_EQ(GameState::Playing, _phase);
	EXPECT_TRUE(_announced.empty());
}

// applying the mode again is a new match: the phase is announced again, and the old match's peers are dropped
TEST_F(NetworkGameStateTest, ApplyingTheModeAgainStartsAFreshMatch)
{
	struct Again final
	{
		std::string_view why;
		GameMode mode;
		std::vector<PlayerSlot> readies{};
		GameState fresh;
	};

	const std::array again{Again{.why = "the same phase name, a new match", .mode = GameMode::OnePlayer,
								 .fresh = GameState::Playing},
						   Again{.why = "the peers of the old match are gone",
								 .mode = GameMode::PlayAsHost,
								 .readies = {PlayerSlot::P1, PlayerSlot::P2},
								 .fresh = GameState::Lobby}};
	for (const Again& match: again)
	{
		SCOPED_TRACE(match.why);
		_stateManager = std::make_unique<GameStateManager>(_events);
		_events->EmitEvent(GameModeAppliedEvent{.mode = match.mode});
		for (const PlayerSlot slot: match.readies)
		{
			_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = slot});
		}
		ASSERT_EQ(GameState::Playing, _phase);
		_announced.clear();

		_events->EmitEvent(GameModeAppliedEvent{.mode = match.mode});

		EXPECT_EQ(match.fresh, _phase);
		EXPECT_EQ(std::vector{match.fresh}, _announced);
	}
}

//NOTE: a client dropping before its ready gave back a seat it never took - the one who stayed kept playing
TEST_F(NetworkGameStateTest, ALossBeforeTheReadyTakesNoSeatAway)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P2});
	ASSERT_EQ(GameState::Playing, _phase);

	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P2});

	EXPECT_EQ(GameState::Playing, _phase);
}

// a restart empties both seats, so one ready player waits again
TEST_F(NetworkGameStateTest, ARestartPutsTheServerBackToWaitingForBothSeats)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	ASSERT_EQ(GameState::Playing, _phase);

	_events->EmitEvent(ServerInRestartRequestedEvent{});
	EXPECT_EQ(GameState::Lobby, _phase);

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	EXPECT_EQ(GameState::Lobby, _phase) << "one player restarted the match alone";

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	EXPECT_EQ(GameState::Playing, _phase);
}

//NOTE: hearing the lobby is what makes a client drop its world - announced once, even from it, and never left again
TEST_F(NetworkGameStateTest, ARestartAnnouncesTheLobbyOnce)
{
	struct Restart final
	{
		std::string_view why;
		MatchSettings match{};
		std::vector<PlayerSlot> readies{};
		GameState before;
	};

	const PauseSwitch pauseSwitch{_events};
	const std::array restarts{
			Restart{.why = "already in the lobby", .match = {}, .before = GameState::Lobby},
			Restart{.why = "a running match",
					.match = {},
					.readies = {PlayerSlot::P1, PlayerSlot::P2},
					.before = GameState::Playing},
			Restart{.why = "the hold let go on the way must not resume the match torn down",
					.match = {.seats = 2u, .isStartingAtOnce = true},
					.readies = {PlayerSlot::P1, PlayerSlot::P2},
					.before = GameState::Paused}};
	for (const Restart& restart: restarts)
	{
		SCOPED_TRACE(restart.why);
		_stateManager = std::make_unique<GameStateManager>(_events, restart.match);
		_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
		for (const PlayerSlot slot: restart.readies)
		{
			_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = slot});
		}
		ASSERT_EQ(restart.before, _phase);
		_announced.clear();

		_events->EmitEvent(ServerInRestartRequestedEvent{});

		EXPECT_EQ(std::vector{GameState::Lobby}, _announced);
	}
}

// the host lifting a pause resumes the client where it stood - a match start would empty the field and reload it
TEST_F(NetworkGameStateTest, AHostLeavingAPauseStartsNoMatch)
{
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsClient});
	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});
	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Paused});
	_announced.clear();
	_matchStarts = 0;

	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});

	EXPECT_EQ(GameState::Playing, _phase);
	EXPECT_EQ(std::vector{GameState::Playing}, _announced);
	EXPECT_EQ(_matchStarts, 0);
}

// the seated client that went away was all the match waited for
TEST_F(NetworkGameStateTest, ASeatedClientLeavingStartsTheMatchThatWaitedForIt)
{
	_stateManager = std::make_unique<GameStateManager>(_events, MatchSettings{.seats = 4u, .isStartingAtOnce = true});
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerClientSeatedEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	ASSERT_EQ(GameState::Lobby, _phase);

	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P2});

	EXPECT_EQ(GameState::Playing, _phase);
}

//NOTE: the top ones - a player joining later sits in an empty seat, and only then takes a bot's
TEST_F(NetworkGameStateTest, TheBotsTakeTheTopSeatsNobodyTookThenTheTopPlayersSeat)
{
	struct Seating final
	{
		std::string_view why;
		MatchSettings match{};
		std::vector<PlayerSlot> seated{};
		std::vector<PlayerSlot> readies{};
		std::array<SeatHolder, kSeatCount> holders;
	};

	std::optional<SeatsFilledEvent> filled{};
	const EventSubscription filledSub{_events->AddListener([&filled](const SeatsFilledEvent& event)
	{
		filled = event;
	})};
	const std::array seatings{
			Seating{.why = "the empty seats first",
					.match = {.seats = 4u, .bots = 2u, .isStartingAtOnce = true},
					.readies = {PlayerSlot::P1},
					.holders = {SeatHolder::Player, SeatHolder::Empty, SeatHolder::Bot, SeatHolder::Bot}},
			Seating{.why = "a bot left over takes the top player's seat",
					.match = {.seats = 4u, .bots = 3u, .isStartingAtOnce = true},
					.seated = {PlayerSlot::P4},
					.readies = {PlayerSlot::P1, PlayerSlot::P4},
					.holders = {SeatHolder::Player, SeatHolder::Bot, SeatHolder::Bot, SeatHolder::Bot}}};
	for (const Seating& seating: seatings)
	{
		SCOPED_TRACE(seating.why);
		filled.reset();
		_stateManager = std::make_unique<GameStateManager>(_events, seating.match);
		_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
		for (const PlayerSlot slot: seating.seated)
		{
			_events->EmitEvent(ServerClientSeatedEvent{.slot = slot});
		}

		for (const PlayerSlot slot: seating.readies)
		{
			_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = slot});
		}

		ASSERT_TRUE(filled.has_value()) << "the match started without saying who sits where";
		EXPECT_EQ(filled->holders, seating.holders);
	}
}

// a player who took their tank keeps it at the next level - the bot left over does not take it back
TEST_F(NetworkGameStateTest, ATankTakenFromABotStaysWithItsPlayerAtTheNextLevel)
{
	std::optional<SeatsFilledEvent> filled{};
	const EventSubscription filledSub{_events->AddListener([&filled](const SeatsFilledEvent& event)
	{
		filled = event;
	})};
	_stateManager = std::make_unique<GameStateManager>(
			_events, MatchSettings{.seats = 2u, .bots = 2u, .isStartingAtOnce = true});
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	ASSERT_TRUE(filled.has_value());
	ASSERT_EQ(filled->holders[SeatIndex(PlayerSlot::P1)], SeatHolder::Bot) << "the second bot left the player alone";
	_events->EmitEvent(ServerInTankCommandEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerClientSyncedEvent{.slot = PlayerSlot::P1});

	_events->EmitEvent(ServerInRestartRequestedEvent{});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});

	EXPECT_EQ(filled->holders, (std::array{SeatHolder::Player, SeatHolder::Bot, SeatHolder::Empty, SeatHolder::Empty}));
}

// a player joining a running match watches the bot in their seat, and takes the tank with their first command
TEST_F(NetworkGameStateTest, AJoinTakesTheTankFromItsBotOnTheFirstCommand)
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
	ASSERT_EQ(GameState::Playing, _phase);
	ASSERT_FALSE(taken.has_value());
	_matchStarts = 0;

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(ServerClientSyncedEvent{.slot = PlayerSlot::P2});
	ASSERT_FALSE(taken.has_value()) << "the tank was taken before the newcomer gave it a command";

	_events->EmitEvent(ServerInTankCommandEvent{.slot = PlayerSlot::P2});

	ASSERT_TRUE(taken.has_value());
	EXPECT_EQ(taken->slot, PlayerSlot::P2);
	EXPECT_EQ(taken->from, SeatHolder::Bot);
	EXPECT_EQ(_matchStarts, 0) << "the join restarted the match";
}

// the match stands still from the join until the newcomer has the field - an unpause meanwhile gets the pause back
TEST_F(NetworkGameStateTest, AJoinHoldsTheMatchUntilTheFieldReachesTheNewcomer)
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

	_events->EmitEvent(SetPauseEvent{.isPaused = false});
	EXPECT_EQ(pauses, (std::vector{true, true})) << "the unpause was not answered with the pause that still stands";

	_events->EmitEvent(ServerClientSyncedEvent{.slot = PlayerSlot::P2});
	EXPECT_EQ(pauses, (std::vector{true, true, false}));
}

// two newcomers at once - the first one caught up does not let the match go for the other
TEST_F(NetworkGameStateTest, TheHoldWaitsForEveryNewcomer)
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

//NOTE: a player pausing while the field is on its way meant it - the end of the sync does not lift it
TEST_F(NetworkGameStateTest, APauseAskedForDuringTheHoldOutlastsIt)
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
	EXPECT_EQ(GameState::Paused, _phase);
}

// a newcomer lost on its way in is asked about like anyone who left, and holds nobody up once answered
TEST_F(NetworkGameStateTest, ANewcomerLostBeforeItSyncedLetsTheMatchGoOn)
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
	EXPECT_EQ(GameState::Playing, _phase);
}

// a player leaving a running match holds it and asks the ones still in - the pause key does not answer the panel
TEST_F(NetworkGameStateTest, APlayerLeavingHoldsTheMatchAndAsksTheRest)
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
	ASSERT_EQ(GameState::Playing, _phase);

	_events->EmitEvent(ServerInDisconnectEvent{.reason = DisconnectReason::GameOver, .slot = PlayerSlot::P2});

	ASSERT_TRUE(asked.has_value());
	EXPECT_EQ(asked->seats, (std::array{Absence::None, Absence::Left, Absence::None, Absence::None}));
	EXPECT_EQ(GameState::Paused, _phase);

	_events->EmitEvent(SetPauseEvent{.isPaused = false});

	EXPECT_EQ(GameState::Paused, _phase) << "an unpause answered the panel";
}

// an answer lets the match go and nobody is asked any more - playing on keeps the seat for the one who left, not a bot
TEST_F(NetworkGameStateTest, AnAnswerLetsTheMatchGoAndSaysWhoTakesTheSeat)
{
	struct Answer final
	{
		AbsenceChoice choice;
		SeatHolder to;
	};

	const PauseSwitch pauseSwitch{_events};
	std::optional<AbsenceChangedEvent> asked{};
	std::vector<SeatHolderChangedEvent> changed{};
	const EventSubscription askedSub{_events->AddListener([&asked](const AbsenceChangedEvent& event)
	{
		asked = event;
	})};
	const EventSubscription changedSub{_events->AddListener([&changed](const SeatHolderChangedEvent& event)
	{
		changed.push_back(event);
	})};
	constexpr std::array kAnswers{Answer{.choice = AbsenceChoice::Continue, .to = SeatHolder::Empty},
								  Answer{.choice = AbsenceChoice::Bot, .to = SeatHolder::Bot}};
	for (const Answer& answer: kAnswers)
	{
		SCOPED_TRACE(static_cast<int>(answer.choice));
		_stateManager = std::make_unique<GameStateManager>(_events);
		_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
		_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
		_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
		_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P2});
		asked.reset();
		changed.clear();

		_events->EmitEvent(AbsenceChosenEvent{.choice = answer.choice});

		ASSERT_TRUE(asked.has_value());
		EXPECT_EQ(asked->seats, (std::array<Absence, kSeatCount>{}));
		EXPECT_EQ(GameState::Playing, _phase);
		ASSERT_EQ(changed.size(), 1u);
		EXPECT_EQ(changed.front().slot, PlayerSlot::P2);
		EXPECT_EQ(changed.front().from, SeatHolder::Player);
		EXPECT_EQ(changed.front().to, answer.to);
	}
}

//NOTE: the one who comes back is shown, but the match goes on only once somebody says so - even after the sync
TEST_F(NetworkGameStateTest, TheOneBackIsShownAndTheMatchWaitsForAnAnswer)
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
	ASSERT_EQ(GameState::Paused, _phase) << "the match went on before anyone said so";

	_events->EmitEvent(AbsenceChosenEvent{.choice = AbsenceChoice::Continue});
	EXPECT_EQ(GameState::Playing, _phase);
}

// a bot takes over the seat of the one who left, and gives it back to the player's first command once they return
TEST_F(NetworkGameStateTest, ABotHoldsTheSeatOfTheOneWhoLeftUntilTheirFirstCommandBack)
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
	ASSERT_EQ(GameState::Playing, _phase);

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(ServerClientSyncedEvent{.slot = PlayerSlot::P2});
	ASSERT_EQ(taken.size(), 1u) << "the bot gave the seat up before its player gave a command";

	_events->EmitEvent(ServerInTankCommandEvent{.slot = PlayerSlot::P2});

	ASSERT_EQ(taken.size(), 2u);
	EXPECT_EQ(taken.back().from, SeatHolder::Bot);
	EXPECT_EQ(taken.back().to, SeatHolder::Player);
}

// the first answer decides - a second one, sent before the first was heard, finds nobody asked about
TEST_F(NetworkGameStateTest, OnlyTheFirstAnswerCounts)
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
TEST_F(NetworkGameStateTest, ALeaveOnTheScoreboardAsksNothing)
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
TEST_F(NetworkGameStateTest, TheLastOneLeavingTakesTheHeldMatchToTheLobby)
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
TEST_F(NetworkGameStateTest, AClientsAnswerChangesNothingOnTheClient)
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
TEST_F(NetworkGameStateTest, ARestartForgetsWhoLeft)
{
	const PauseSwitch pauseSwitch{_events};
	bool isPaused{};
	const EventSubscription pauseSub{_events->AddListener([&isPaused](const PauseStatusEvent& event)
	{
		isPaused = event.isPaused;
	})};
	_stateManager = std::make_unique<GameStateManager>(_events, MatchSettings{.seats = 3u});
	_events->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
	constexpr std::array kLeavers{PlayerSlot::P1, PlayerSlot::P2, PlayerSlot::P3};
	for (const PlayerSlot slot: kLeavers)
	{
		_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = slot});
	}
	_events->EmitEvent(ServerClientLostEvent{.slot = PlayerSlot::P3});
	ASSERT_TRUE(isPaused);

	_events->EmitEvent(ServerInRestartRequestedEvent{});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P3});

	EXPECT_EQ(GameState::Playing, _phase);
	EXPECT_FALSE(isPaused) << "the new match is held for the one who left the old one";
}
