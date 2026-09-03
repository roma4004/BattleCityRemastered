#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include <memory>

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
		const auto bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, _gameConfig.gameMode, _respawnManager,
								 _tankSpawner);
		_events->EmitEvent(GameResetEvent{});
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
	}

	void TearDown() override {}
};

TEST_F(RespawnManagerTest, EnemyDiedRespawnCount)
{
	constexpr unsigned short respawnOriginal{20u};
	unsigned short respawnActual{20u};
	auto respawnSub = _events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::ENEMY_ALL)
		{
			respawnActual = event.respawnCount;
		}
	});

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::OnePlayer, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
	_allObjects.pop_back();

	EXPECT_GT(respawnOriginal, respawnActual);

}

TEST_F(RespawnManagerTest, PlayerOneDiedRespawnCount)
{
	constexpr unsigned short respawnOriginal{3u};
	unsigned short respawnActual{3u};
	auto respawnSub = _events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::PLAYER1)
		{
			respawnActual = event.respawnCount;
		}
	});

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::OnePlayer, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
	_allObjects.pop_back();

	EXPECT_GT(respawnOriginal, respawnActual);

}

TEST_F(RespawnManagerTest, PlayerTwoDiedRespawnCount)
{
	constexpr unsigned short respawnOriginal{3u};
	unsigned short respawnActual{3u};
	auto respawnSub = _events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::PLAYER2)
		{
			respawnActual = event.respawnCount;
		}
	});

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::TwoPlayers, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
	_allObjects.pop_back();

	EXPECT_GT(respawnOriginal, respawnActual);

}

TEST_F(RespawnManagerTest, EnemyRunOutRespawnPoints)
{
	constexpr unsigned short respawnOriginal{20u};
	unsigned short respawnActual{20u};
	auto respawnSub = _events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::ENEMY_ALL)
		{
			respawnActual = event.respawnCount;
		}
	});

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

TEST_F(RespawnManagerTest, PlayerOneRunOutRespawnPoints)
{
	constexpr unsigned short respawnOriginal{3u};
	unsigned short respawnActual{3u};
	auto respawnSub = _events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::PLAYER1)
		{
			respawnActual = event.respawnCount;
		}
	});

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

TEST_F(RespawnManagerTest, PlayerTwoRunOutRespawnPoints)
{
	constexpr unsigned short respawnOriginal{3u};
	unsigned short respawnActual{3u};
	auto respawnSub = _events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::PLAYER2)
		{
			respawnActual = event.respawnCount;
		}
	});

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

TEST_F(RespawnManagerTest, EnemyRunOutRespawnPointsAndTryMore)
{
	constexpr unsigned short respawnOriginal{21u};
	unsigned short respawnActual{21u};
	auto respawnSub = _events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::ENEMY_ALL)
		{
			respawnActual = event.respawnCount;
		}
	});

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

TEST_F(RespawnManagerTest, PlayerOneRunOutRespawnPointsAndTryMore)
{
	constexpr unsigned short respawnOriginal{4u};
	unsigned short respawnActual{3u};
	auto respawnSub = _events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::PLAYER1)
		{
			respawnActual = event.respawnCount;
		}
	});

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

TEST_F(RespawnManagerTest, PlayerTwoRunOutRespawnPointsAndTryMore)
{
	constexpr unsigned short respawnOriginal{4u};
	unsigned short respawnActual{3u};
	auto respawnSub = _events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::PLAYER2)
		{
			respawnActual = event.respawnCount;
		}
	});

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
