#include "application/GameConfig.h"
#include "application/Simulation.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/TimingEvents.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include <gtest/gtest.h>
#include <algorithm>
#include <memory>
#include <string>
#include <vector>

// The host answers a ready with the whole world, so a ready sent before the client clears its own
// erases exactly the world it asked for.
class ClientHandshakeTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{std::make_shared<EventSystem>()};
	GameConfig _gameConfig{};
	Simulation _simulation{_events, _gameConfig};

	std::vector<std::string> _trace{};
	std::vector<EventSubscription> _subs{};

	void SetUp() override
	{
		_subs.push_back(_events->AddListener([this](const GameResetEvent&) { _trace.emplace_back("reset"); }));
		_subs.push_back(_events->AddListener([this](const ClientOutReadyToPlayEvent&)
		{
			_trace.emplace_back("ready");
		}));
	}

	//NOTE: the lobby is entered at PostTickUpdate, which only a frame produces
	void Frame()
	{
		_events->EmitEvent(DeltaTimeEvent{.deltaTime = 1.0 / 60.0});
		_simulation.Tick();
	}

	[[nodiscard]] std::ptrdiff_t Count(const std::string_view what) const
	{
		return std::ranges::count(_trace, what);
	}
};

TEST_F(ClientHandshakeTest, TheReadyFollowsTheWorldReset)
{
	_simulation.ApplyGameMode(GameMode::PlayAsClient);
	Frame();
	_trace.clear();

	_events->EmitEvent(ClientConnectedToHostEvent{});
	Frame();

	EXPECT_EQ(_trace, (std::vector<std::string>{"reset", "ready"}));
}

//NOTE: the announcement would be dropped, and the client would sit out the match it asked for
TEST_F(ClientHandshakeTest, NothingIsAnnouncedBeforeTheLinkIsUp)
{
	_simulation.ApplyGameMode(GameMode::PlayAsClient);
	Frame();
	Frame();

	EXPECT_EQ(Count("ready"), 0);
}

//NOTE: the host counts readies - two from one client fill both seats and start a match alone
TEST_F(ClientHandshakeTest, TheLinkArrivingWithTheLobbyStillAnnouncesOnce)
{
	_simulation.ApplyGameMode(GameMode::PlayAsClient);
	_events->EmitEvent(ClientConnectedToHostEvent{});
	Frame();
	Frame();

	EXPECT_EQ(Count("ready"), 1);
}

TEST_F(ClientHandshakeTest, TheHostsLobbyMakesAConnectedClientReportAgain)
{
	_simulation.ApplyGameMode(GameMode::PlayAsClient);
	_events->EmitEvent(ClientConnectedToHostEvent{});
	Frame();
	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Playing});
	Frame();
	_trace.clear();

	_events->EmitEvent(HostPhaseAnnouncedEvent{.phase = GameState::Lobby});
	Frame();

	EXPECT_EQ(_trace, (std::vector<std::string>{"reset", "ready"}));
}

//NOTE: a lost link leaves the phase in the lobby, and announcing into it would only be dropped
TEST_F(ClientHandshakeTest, ALostLinkStopsTheAnnouncements)
{
	_simulation.ApplyGameMode(GameMode::PlayAsClient);
	_events->EmitEvent(ClientConnectedToHostEvent{});
	Frame();
	_trace.clear();

	_events->EmitEvent(ClientReconnectAbandonedEvent{});
	Frame();

	EXPECT_EQ(Count("ready"), 0);
}
