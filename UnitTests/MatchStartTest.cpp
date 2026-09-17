#include "application/GameConfig.h"
#include "application/Simulation.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/SpawnEvents.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include <memory>
#include <string_view>
#include <vector>

class MatchStartTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{std::make_shared<EventSystem>()};
	GameConfig _gameConfig{};
	Simulation _simulation{_events, _gameConfig};

	std::vector<std::string_view> _order{};
	std::vector<EventSubscription> _subs{};

	void SetUp() override
	{
		//NOTE: the spawners are built on the mode change, so this is what gives the match a map to load
		_simulation.ApplyGameMode(GameMode::OnePlayer);

		_subs.push_back(_events->AddListener([this](const GameResetEvent&) { _order.emplace_back("reset"); }));
		_subs.push_back(_events->AddListener([this](const AddToSpawnQueueEvent&)
		{
			if (_order.empty() || _order.back() != "spawn")
			{
				_order.emplace_back("spawn");
			}
		}));
	}
};

//NOTE: the reset and the map load used to hang off MatchStartedEvent, ordered only by who subscribed first
TEST_F(MatchStartTest, TheMapFillsTheFieldTheResetJustEmptied)
{
	_events->EmitEvent(MatchStartedEvent{});

	EXPECT_EQ(_order, (std::vector<std::string_view>{"reset", "spawn"}));
}
