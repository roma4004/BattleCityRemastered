#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "entities/pawns/Tank.h"
#include "enums/Author.h"
#include "enums/Faction.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include "gtest/gtest.h"
#include <memory>
#include <optional>
#include <ranges>
#include <vector>

// who is owed a respawn and how many lives are left: a death is announced and the counter of that seat is read
class RespawnManagerTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<RespawnManager> _respawnManager{nullptr};
	std::vector<EventSubscription> _instantSpawnAnimationSubs{};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_allObjects.reserve(6u);
		const auto bulletPool{std::make_shared<BulletPool>(_events, _allObjects, _gameConfig)};
		TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, _gameConfig.gameMode, _respawnManager,
								 _tankSpawner);
		_events->EmitEvent(GameResetEvent{});
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
	}

	void TearDown() override {}
};

// a dead bot takes one off the enemy team's count
TEST_F(RespawnManagerTest, EnemyDiedRespawnCount)
{
	constexpr unsigned short respawnOriginal{20u};
	unsigned short respawnActual{20u};
	auto respawnSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::ENEMY_ALL)
		{
			respawnActual = event.respawnCount;
		}
	})};

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::OnePlayer, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
	_allObjects.pop_back();

	EXPECT_GT(respawnOriginal, respawnActual);

}

// a dead player one off its own
TEST_F(RespawnManagerTest, PlayerOneDiedRespawnCount)
{
	constexpr unsigned short respawnOriginal{3u};
	unsigned short respawnActual{3u};
	auto respawnSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::PLAYER1)
		{
			respawnActual = event.respawnCount;
		}
	})};

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::OnePlayer, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
	_allObjects.pop_back();

	EXPECT_GT(respawnOriginal, respawnActual);

}

// and player two off the second seat's
TEST_F(RespawnManagerTest, PlayerTwoDiedRespawnCount)
{
	constexpr unsigned short respawnOriginal{3u};
	unsigned short respawnActual{3u};
	auto respawnSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::PLAYER2)
		{
			respawnActual = event.respawnCount;
		}
	})};

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::TwoPlayers, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
	_allObjects.pop_back();

	EXPECT_GT(respawnOriginal, respawnActual);

}

// spending the last enemy life empties the count
TEST_F(RespawnManagerTest, EnemyRunOutRespawnPoints)
{
	constexpr unsigned short respawnOriginal{20u};
	unsigned short respawnActual{20u};
	auto respawnSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::ENEMY_ALL)
		{
			respawnActual = event.respawnCount;
		}
	})};

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::OnePlayer, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	for (unsigned short i = 0u; i < respawnOriginal; ++i)
	{
		_events->EmitEvent(RespawnTanksEvent{});
		_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
		_allObjects.pop_back();
	}

	EXPECT_EQ(0u, respawnActual);

}

// the same for player one
TEST_F(RespawnManagerTest, PlayerOneRunOutRespawnPoints)
{
	constexpr unsigned short respawnOriginal{3u};
	unsigned short respawnActual{3u};
	auto respawnSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::PLAYER1)
		{
			respawnActual = event.respawnCount;
		}
	})};

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::OnePlayer, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	for (unsigned short i = 0u; i < respawnOriginal; ++i)
	{
		_events->EmitEvent(RespawnTanksEvent{});
		_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
		_allObjects.pop_back();
	}

	EXPECT_EQ(0u, respawnActual);

}

// and for player two
TEST_F(RespawnManagerTest, PlayerTwoRunOutRespawnPoints)
{
	constexpr unsigned short respawnOriginal{3u};
	unsigned short respawnActual{3u};
	auto respawnSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::PLAYER2)
		{
			respawnActual = event.respawnCount;
		}
	})};

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::TwoPlayers, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	for (unsigned short i = 0u; i < respawnOriginal; ++i)
	{
		_events->EmitEvent(RespawnTanksEvent{});
		_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
		_allObjects.pop_back();
	}

	EXPECT_EQ(0u, respawnActual);

}

// dying again past that leaves the count at nothing rather than wrapping around
TEST_F(RespawnManagerTest, EnemyRunOutRespawnPointsAndTryMore)
{
	constexpr unsigned short respawnOriginal{21u};
	unsigned short respawnActual{21u};
	auto respawnSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::ENEMY_ALL)
		{
			respawnActual = event.respawnCount;
		}
	})};

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::OnePlayer, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	for (unsigned short i = 0u; i < respawnOriginal; ++i)
	{
		_events->EmitEvent(RespawnTanksEvent{});
		if (!_allObjects.empty())
		{
			_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
			_allObjects.pop_back();
		}
	}

	EXPECT_EQ(0u, respawnActual);

}

// the same for the first seat
TEST_F(RespawnManagerTest, PlayerOneRunOutRespawnPointsAndTryMore)
{
	constexpr unsigned short respawnOriginal{4u};
	unsigned short respawnActual{3u};
	auto respawnSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::PLAYER1)
		{
			respawnActual = event.respawnCount;
		}
	})};

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::OnePlayer, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	for (unsigned short i = 0u; i < respawnOriginal; ++i)
	{
		_events->EmitEvent(RespawnTanksEvent{});
		if (!_allObjects.empty())
		{
			_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
			_allObjects.pop_back();
		}
	}

	EXPECT_EQ(0u, respawnActual);

}

// and for the second
TEST_F(RespawnManagerTest, PlayerTwoRunOutRespawnPointsAndTryMore)
{
	constexpr unsigned short respawnOriginal{4u};
	unsigned short respawnActual{3u};
	auto respawnSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::PLAYER2)
		{
			respawnActual = event.respawnCount;
		}
	})};

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::TwoPlayers, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	for (unsigned short i = 0u; i < respawnOriginal; ++i)
	{
		_events->EmitEvent(RespawnTanksEvent{});
		if (!_allObjects.empty())
		{
			_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
			_allObjects.pop_back();
		}
	}

	EXPECT_EQ(0u, respawnActual);

}

// 2P free-for-all is won by the last player standing, not by clearing the bots
TEST_F(RespawnManagerTest, TwoPlayersFreeForAllIsWonByTheLastPlayerStanding)
{
	std::optional<GameState> finished{};
	const EventSubscription finishSub{_events->AddListener([&finished](const GameFinishedEvent& event)
	{
		finished = event.state;
	})};
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::TwoPlayersFreeForAll, _respawnManager,
							 _tankSpawner);
	_events->EmitEvent(GameResetEvent{});

	const auto killEvery = [this](const auto& isTarget)
	{
		for (const std::shared_ptr<BaseObj>& tank: _allObjects | std::views::filter(isTarget))
		{
			_events->EmitEvent(TankDiedEvent{.uuid = tank->GetUuid()});
		}

		std::erase_if(_allObjects, isTarget);
	};
	const auto isBot = [](const std::shared_ptr<BaseObj>& obj)
	{
		const auto tank{std::dynamic_pointer_cast<Tank>(obj)};

		return tank != nullptr && FactionOf(tank->GetAuthor()) == Faction::EnemyTeam;
	};
	const auto isPlayerOne = [](const std::shared_ptr<BaseObj>& obj)
	{
		const auto tank{std::dynamic_pointer_cast<Tank>(obj)};

		return tank != nullptr && tank->GetAuthor() == Author::Player1;
	};

	for (int wave{}; wave < 20; ++wave)
	{
		_events->EmitEvent(RespawnTanksEvent{});
		killEvery(isBot);
	}

	ASSERT_FALSE(finished.has_value()) << "the bots were cleared with both players standing, and that ended it";

	for (int life{}; life < 3; ++life)
	{
		_events->EmitEvent(RespawnTanksEvent{});
		killEvery(isPlayerOne);
	}

	EXPECT_EQ(finished, GameState::Won);
}
