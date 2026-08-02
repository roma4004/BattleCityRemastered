#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/TankSpawner.h"
#include "components/managers/DelayedSpawnManager.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include <memory>

class RespawnManagerTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<DelayedSpawnManager> _spawnDelayManager{nullptr};
	GameConfig _gameConfig{"", true};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_allObjects.reserve(6u);
		const auto bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _gameConfig);
		_tankSpawner = std::make_shared<TankSpawner>(_gameConfig, &_allObjects, _events);
		_spawnDelayManager = std::make_shared<DelayedSpawnManager>(_events);
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

TEST_F(RespawnManagerTest, EnemyDiedRespawnCount)
{
	constexpr unsigned short respawnOriginal{20u};
	unsigned short respawnActual{20u};
	_events->AddListener(
			"RespawnCountChangedTo", "TankSpawnerTest",
			[&respawnActual](const std::string& objectName, const unsigned short respawnCount)
			{
				if (objectName == "Enemy")
				{
					respawnActual = respawnCount;
				}
			});

	constexpr bool skipDelay{true};
	_events->EmitEvent("GameModeChangedTo", GameMode::OnePlayer);
	_events->EmitEvent("RespawnTanks", skipDelay);
	_allObjects.pop_back();

	EXPECT_GT(respawnOriginal, respawnActual);

	_events->RemoveListener("RespawnCountChangedTo", "TankSpawnerTest");
}

TEST_F(RespawnManagerTest, PlayerOneDiedRespawnCount)
{
	constexpr unsigned short respawnOriginal{3u};
	unsigned short respawnActual{3u};
	_events->AddListener(
			"RespawnCountChangedTo", "TankSpawnerTest",
			[&respawnActual](const std::string& objectName, const unsigned short respawnCount)
			{
				if (objectName == "Player1")
				{
					respawnActual = respawnCount;
				}
			});

	constexpr bool skipDelay{true};
	_events->EmitEvent("GameModeChangedTo", GameMode::OnePlayer);
	_events->EmitEvent("RespawnTanks", skipDelay);
	_allObjects.pop_back();

	EXPECT_GT(respawnOriginal, respawnActual);

	_events->RemoveListener("RespawnCountChangedTo", "TankSpawnerTest");
}

TEST_F(RespawnManagerTest, PlayerTwoDiedRespawnCount)
{
	constexpr unsigned short respawnOriginal{3u};
	unsigned short respawnActual{3u};
	_events->AddListener(
			"RespawnCountChangedTo", "TankSpawnerTest",
			[&respawnActual](const std::string& objectName, const unsigned short respawnCount)
			{
				if (objectName == "Player2")
				{
					respawnActual = respawnCount;
				}
			});

	constexpr bool skipDelay{true};
	_events->EmitEvent("GameModeChangedTo", GameMode::TwoPlayers);
	_events->EmitEvent("RespawnTanks", skipDelay);
	_allObjects.pop_back();

	EXPECT_GT(respawnOriginal, respawnActual);

	_events->RemoveListener("RespawnCountChangedTo", "TankSpawnerTest");
}

TEST_F(RespawnManagerTest, EnemyRunOutRespawnPoints)
{
	constexpr unsigned short respawnOriginal{20u};
	unsigned short respawnActual{20u};
	_events->AddListener(
			"RespawnCountChangedTo", "TankSpawnerTest",
			[&respawnActual](const std::string& objectName, const unsigned short respawnCount)
			{
				if (objectName == "Enemy")
				{
					respawnActual = respawnCount;
				}
			});

	_events->EmitEvent("GameModeChangedTo", GameMode::OnePlayer);
	for (unsigned short i = 0u; i < respawnOriginal; ++i)
	{
		constexpr bool skipDelay{true};
		_events->EmitEvent("RespawnTanks", skipDelay);
		_allObjects.pop_back();
	}

	EXPECT_EQ(0u, respawnActual);

	_events->RemoveListener("RespawnCountChangedTo", "TankSpawnerTest");
}

TEST_F(RespawnManagerTest, PlayerOneRunOutRespawnPoints)
{
	constexpr unsigned short respawnOriginal{3u};
	unsigned short respawnActual{3u};
	_events->AddListener(
			"RespawnCountChangedTo", "TankSpawnerTest",
			[&respawnActual](const std::string& objectName, const unsigned short respawnCount)
			{
				if (objectName == "Player1")
				{
					respawnActual = respawnCount;
				}
			});

	_events->EmitEvent("GameModeChangedTo", GameMode::OnePlayer);
	for (unsigned short i = 0u; i < respawnOriginal; ++i)
	{
		constexpr bool skipDelay{true};
		_events->EmitEvent("RespawnTanks", skipDelay);
		_allObjects.pop_back();
	}

	EXPECT_EQ(0, respawnActual);

	_events->RemoveListener("RespawnCountChangedTo", "TankSpawnerTest");
}

TEST_F(RespawnManagerTest, PlayerTwoRunOutRespawnPoints)
{
	constexpr unsigned short respawnOriginal{3u};
	unsigned short respawnActual{3u};
	_events->AddListener(
			"RespawnCountChangedTo", "TankSpawnerTest",
			[&respawnActual](const std::string& objectName, const unsigned short respawnCount)
			{
				if (objectName == "Player2")
				{
					respawnActual = respawnCount;
				}
			});

	_events->EmitEvent("GameModeChangedTo", GameMode::TwoPlayers);
	for (unsigned short i = 0u; i < respawnOriginal; ++i)
	{
		constexpr bool skipDelay{true};
		_events->EmitEvent("RespawnTanks", skipDelay);
		_allObjects.pop_back();
	}

	EXPECT_EQ(0, respawnActual);

	_events->RemoveListener("RespawnCountChangedTo", "TankSpawnerTest");
}

TEST_F(RespawnManagerTest, EnemyRunOutRespawnPointsAndTryMore)
{
	constexpr unsigned short respawnOriginal{21u};
	unsigned short respawnActual{21u};
	_events->AddListener(
			"RespawnCountChangedTo", "TankSpawnerTest",
			[&respawnActual](const std::string& objectName, const unsigned short respawnCount)
			{
				if (objectName == "Enemy")
				{
					respawnActual = respawnCount;
				}
			});

	_events->EmitEvent("GameModeChangedTo", GameMode::OnePlayer);
	for (unsigned short i = 0u; i < respawnOriginal; ++i)
	{
		constexpr bool skipDelay{true};
		_events->EmitEvent("RespawnTanks", skipDelay);
		if (!_allObjects.empty())
		{
			_allObjects.pop_back();
		}
	}

	EXPECT_EQ(0, respawnActual);

	_events->RemoveListener("RespawnCountChangedTo", "TankSpawnerTest");
}

TEST_F(RespawnManagerTest, PlayerOneRunOutRespawnPointsAndTryMore)
{
	constexpr unsigned short respawnOriginal{4u};
	unsigned short respawnActual{3u};
	_events->AddListener(
			"RespawnCountChangedTo", "TankSpawnerTest",
			[&respawnActual](const std::string& objectName, const unsigned short respawnCount)
			{
				if (objectName == "Player1")
				{
					respawnActual = respawnCount;
				}
			});

	_events->EmitEvent("GameModeChangedTo", GameMode::OnePlayer);
	for (unsigned short i = 0; i < respawnOriginal; ++i)
	{
		constexpr bool skipDelay{true};
		_events->EmitEvent("RespawnTanks", skipDelay);
		if (!_allObjects.empty())
		{
			_allObjects.pop_back();
		}
	}

	EXPECT_EQ(0, respawnActual);

	_events->RemoveListener("RespawnCountChangedTo", "TankSpawnerTest");
}

TEST_F(RespawnManagerTest, PlayerTwoRunOutRespawnPointsAndTryMore)
{
	constexpr unsigned short respawnOriginal{4u};
	unsigned short respawnActual{3u};
	_events->AddListener(
			"RespawnCountChangedTo", "TankSpawnerTest",
			[&respawnActual](const std::string& objectName, const unsigned short respawnCount)
			{
				if (objectName == "Player2")
				{
					respawnActual = respawnCount;
				}
			});


	_events->EmitEvent("GameModeChangedTo", GameMode::TwoPlayers);
	for (unsigned short i = 0u; i < respawnOriginal; ++i)
	{
		constexpr bool skipDelay{true};
		_events->EmitEvent("RespawnTanks", skipDelay);
		if (!_allObjects.empty())
		{
			_allObjects.pop_back();
		}
	}

	EXPECT_EQ(0, respawnActual);

	_events->RemoveListener("RespawnCountChangedTo", "TankSpawnerTest");
}
