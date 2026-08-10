#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/SpawnEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/TankSpawner.h"
#include "components/managers/DelayedSpawnManager.h"
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
	std::shared_ptr<DelayedSpawnManager> _spawnDelayManager{nullptr};
	GameConfig _gameConfig{"", true};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, &_allObjects);
		_allObjects.reserve(6u);
		const auto bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _gameConfig);
		_respawnManager = std::make_shared<RespawnManager>(_events);
		_tankSpawner = std::make_shared<TankSpawner>(_gameConfig, &_allObjects, _events);
		_spawnDelayManager = std::make_shared<DelayedSpawnManager>(_events);
	}

	void TearDown() override
	{
	}
};

TEST_F(RespawnManagerTest, EnemyDiedRespawnCount)
{
	constexpr unsigned short respawnOriginal{20u};
	unsigned short respawnActual{20u};
	auto respawnSub = _events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.objectName == "Enemy")
		{
			respawnActual = event.respawnCount;
		}
	});

	constexpr bool skipDelay{true};
	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::OnePlayer});
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	_allObjects.pop_back();

	EXPECT_GT(respawnOriginal, respawnActual);

}

TEST_F(RespawnManagerTest, PlayerOneDiedRespawnCount)
{
	constexpr unsigned short respawnOriginal{3u};
	unsigned short respawnActual{3u};
	auto respawnSub = _events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.objectName == "Player1")
		{
			respawnActual = event.respawnCount;
		}
	});

	constexpr bool skipDelay{true};
	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::OnePlayer});
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	_allObjects.pop_back();

	EXPECT_GT(respawnOriginal, respawnActual);

}

TEST_F(RespawnManagerTest, PlayerTwoDiedRespawnCount)
{
	constexpr unsigned short respawnOriginal{3u};
	unsigned short respawnActual{3u};
	auto respawnSub = _events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.objectName == "Player2")
		{
			respawnActual = event.respawnCount;
		}
	});

	constexpr bool skipDelay{true};
	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::TwoPlayers});
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	_allObjects.pop_back();

	EXPECT_GT(respawnOriginal, respawnActual);

}

TEST_F(RespawnManagerTest, EnemyRunOutRespawnPoints)
{
	constexpr unsigned short respawnOriginal{20u};
	unsigned short respawnActual{20u};
	auto respawnSub = _events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.objectName == "Enemy")
		{
			respawnActual = event.respawnCount;
		}
	});

	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::OnePlayer});
	for (unsigned short i = 0u; i < respawnOriginal; ++i)
	{
		constexpr bool skipDelay{true};
		_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
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
		if (event.objectName == "Player1")
		{
			respawnActual = event.respawnCount;
		}
	});

	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::OnePlayer});
	for (unsigned short i = 0u; i < respawnOriginal; ++i)
	{
		constexpr bool skipDelay{true};
		_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
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
		if (event.objectName == "Player2")
		{
			respawnActual = event.respawnCount;
		}
	});

	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::TwoPlayers});
	for (unsigned short i = 0u; i < respawnOriginal; ++i)
	{
		constexpr bool skipDelay{true};
		_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
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
		if (event.objectName == "Enemy")
		{
			respawnActual = event.respawnCount;
		}
	});

	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::OnePlayer});
	for (unsigned short i = 0u; i < respawnOriginal; ++i)
	{
		constexpr bool skipDelay{true};
		_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
		if (!_allObjects.empty())
		{
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
		if (event.objectName == "Player1")
		{
			respawnActual = event.respawnCount;
		}
	});

	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::OnePlayer});
	for (unsigned short i = 0u; i < respawnOriginal; ++i)
	{
		constexpr bool skipDelay{true};
		_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
		if (!_allObjects.empty())
		{
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
		if (event.objectName == "Player2")
		{
			respawnActual = event.respawnCount;
		}
	});

	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::TwoPlayers});
	for (unsigned short i = 0u; i < respawnOriginal; ++i)
	{
		constexpr bool skipDelay{true};
		_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
		if (!_allObjects.empty())
		{
			_allObjects.pop_back();
		}
	}

	EXPECT_EQ(0u, respawnActual);

}
