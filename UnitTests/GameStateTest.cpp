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
	EventSubscription _stateSub{};

	void SetUp() override
	{
		_stateManager = std::make_unique<GameStateManager>(_events);
		_stateSub = _events->AddListener([this](const GameStateChangedToEvent& event)
		{
			_announced.push_back(event.state);
		});
	}

	void ApplyGameMode(const GameMode gameMode) const
	{
		_events->EmitEvent(GameModeAppliedEvent{.mode = gameMode});
	}

	[[nodiscard]] GameState State() const { return _stateManager->GetState(); }
};

TEST_F(GameStateTest, LocalGameStartsPlayingWithNoOneToWaitFor)
{
	ApplyGameMode(GameMode::OnePlayer);

	EXPECT_EQ(GameState::Playing, State());
	EXPECT_EQ(std::vector{GameState::Playing}, _announced);
}

TEST_F(GameStateTest, NetworkGameStartsInTheLobby)
{
	ApplyGameMode(GameMode::PlayAsHost);

	EXPECT_EQ(GameState::Lobby, State());
}

TEST_F(GameStateTest, HostLeavesTheLobbyWhenTheClientIsReady)
{
	ApplyGameMode(GameMode::PlayAsHost);
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{});

	EXPECT_EQ(GameState::Playing, State());
}

TEST_F(GameStateTest, ClientLeavesTheLobbyOnceTheLinkIsUp)
{
	ApplyGameMode(GameMode::PlayAsClient);
	_events->EmitEvent(ClientConnectedToHostEvent{});

	EXPECT_EQ(GameState::Playing, State());
}

TEST_F(GameStateTest, EveryKindOfPeerLossGoesBackToTheLobby)
{
	ApplyGameMode(GameMode::PlayAsHost);

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{});
	ASSERT_EQ(GameState::Playing, State());
	_events->EmitEvent(ServerInDisconnectEvent{.reason = DisconnectReason::PlayerQuit});
	EXPECT_EQ(GameState::Lobby, State());

	_events->EmitEvent(ServerInClientReadyToStartGameEvent{});
	ASSERT_EQ(GameState::Playing, State());
	_events->EmitEvent(ServerClientLostEvent{});
	EXPECT_EQ(GameState::Lobby, State());

	ApplyGameMode(GameMode::PlayAsClient);

	_events->EmitEvent(ClientConnectedToHostEvent{});
	ASSERT_EQ(GameState::Playing, State());
	_events->EmitEvent(ClientInDisconnectEvent{.reason = DisconnectReason::HostShutdown});
	EXPECT_EQ(GameState::Lobby, State());

	_events->EmitEvent(ClientConnectedToHostEvent{});
	ASSERT_EQ(GameState::Playing, State());
	_events->EmitEvent(ClientReconnectAbandonedEvent{});
	EXPECT_EQ(GameState::Lobby, State());
}

TEST_F(GameStateTest, ReconnectingAfterALossStartsTheMatchAgain)
{
	ApplyGameMode(GameMode::PlayAsClient);
	_events->EmitEvent(ClientConnectedToHostEvent{});
	_events->EmitEvent(ClientReconnectAbandonedEvent{});
	ASSERT_EQ(GameState::Lobby, State());

	_announced.clear();
	_events->EmitEvent(ClientConnectedToHostEvent{});

	EXPECT_EQ(GameState::Playing, State());
	EXPECT_EQ(std::vector{GameState::Playing}, _announced);
}

TEST_F(GameStateTest, ALocalGameNeverEntersTheLobby)
{
	ApplyGameMode(GameMode::OnePlayer);
	_events->EmitEvent(ServerClientLostEvent{});

	EXPECT_EQ(GameState::Playing, State());
}

TEST_F(GameStateTest, PauseOnlyTogglesWhileTheMatchRuns)
{
	ApplyGameMode(GameMode::OnePlayer);

	_events->EmitEvent(PauseStatusEvent{.isPaused = true});
	EXPECT_EQ(GameState::Paused, State());

	_events->EmitEvent(PauseStatusEvent{.isPaused = false});
	EXPECT_EQ(GameState::Playing, State());
}

TEST_F(GameStateTest, PausingInTheLobbyKeepsTheLobby)
{
	ApplyGameMode(GameMode::PlayAsHost);

	_events->EmitEvent(PauseStatusEvent{.isPaused = true});

	EXPECT_EQ(GameState::Lobby, State());
}

TEST_F(GameStateTest, WinAndLossSurviveUntilTheFieldIsCleared)
{
	ApplyGameMode(GameMode::OnePlayer);

	_events->EmitEvent(PlayersTeamIsWonEvent{});
	EXPECT_EQ(GameState::Won, State());
	_events->EmitEvent(PauseStatusEvent{.isPaused = true});
	EXPECT_EQ(GameState::Won, State()) << "a pause key press erased the result";

	_events->EmitEvent(GameResetEvent{});
	EXPECT_EQ(GameState::Playing, State());

	_events->EmitEvent(EnemiesTeamIsWonEvent{});
	EXPECT_EQ(GameState::Over, State());
}

//NOTE: entering a phase clears the field, so answering the reset with a phase change would loop
TEST_F(GameStateTest, AResetDuringAMatchChangesNothing)
{
	ApplyGameMode(GameMode::PlayAsHost);
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{});
	ASSERT_EQ(GameState::Playing, State());

	_announced.clear();
	_events->EmitEvent(GameResetEvent{});

	EXPECT_EQ(GameState::Playing, State());
	EXPECT_TRUE(_announced.empty());
}

//NOTE: same phase name, new match - the entry still has to be announced
TEST_F(GameStateTest, RestartingTheSameModeAnnouncesThePhaseAgain)
{
	ApplyGameMode(GameMode::OnePlayer);
	_announced.clear();

	ApplyGameMode(GameMode::OnePlayer);

	EXPECT_EQ(std::vector{GameState::Playing}, _announced);
}

TEST_F(GameStateTest, LeavingANetworkGameForgetsThePeer)
{
	ApplyGameMode(GameMode::PlayAsHost);
	_events->EmitEvent(ServerInClientReadyToStartGameEvent{});
	ASSERT_EQ(GameState::Playing, State());

	ApplyGameMode(GameMode::PlayAsHost);

	EXPECT_EQ(GameState::Lobby, State()) << "a new match kept the peer of the old one";
}
