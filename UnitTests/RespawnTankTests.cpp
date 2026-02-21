#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/TankSpawner.h"
#include "components/managers/DelayedSpawnManager.h"
#include "enums/GameMode.h"
#include "enums/TankType.h"
#include "gtest/gtest.h"
#include <memory>

class TankSpawnerTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<DelayedSpawnManager> _spawnDelayManager{nullptr};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_allObjects.reserve(6);
		constexpr UPoint windowSize{.x = 800, .y = 600};
		const auto bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, windowSize, GameMode::OnePlayer);
		_tankSpawner = std::make_shared<TankSpawner>(windowSize, &_allObjects, _events);
		_spawnDelayManager = std::make_shared<DelayedSpawnManager>(_events);
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

TEST_F(TankSpawnerTest, EnemyOneRespawn)
{
	constexpr bool skipDelay = true;
	_tankSpawner->RespawnTanks(skipDelay);
	EXPECT_EQ(_allObjects.size(), 0);

	_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::ENEMY1));

	_tankSpawner->RespawnTanks(skipDelay);
	EXPECT_EQ(_allObjects.size(), 1);
}

TEST_F(TankSpawnerTest, EnemyTwoRespawn)
{
	constexpr bool skipDelay = true;
	_tankSpawner->RespawnTanks(skipDelay);
	EXPECT_EQ(_allObjects.size(), 0);

	_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::ENEMY2));

	_tankSpawner->RespawnTanks(skipDelay);
	EXPECT_EQ(_allObjects.size(), 1);
}

TEST_F(TankSpawnerTest, EnemyThreeRespawn)
{
	constexpr bool skipDelay = true;
	_tankSpawner->RespawnTanks(skipDelay);
	EXPECT_EQ(_allObjects.size(), 0);

	_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::ENEMY3));

	_tankSpawner->RespawnTanks(skipDelay);
	EXPECT_EQ(_allObjects.size(), 1);
}

TEST_F(TankSpawnerTest, EnemyFourRespawn)
{
	constexpr bool skipDelay = true;
	_tankSpawner->RespawnTanks(skipDelay);
	EXPECT_EQ(_allObjects.size(), 0);

	_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::ENEMY4));

	_tankSpawner->RespawnTanks(skipDelay);
	EXPECT_EQ(_allObjects.size(), 1);
}

TEST_F(TankSpawnerTest, PlayerOneDiedRespawn)
{
	constexpr bool skipDelay = true;
	_tankSpawner->RespawnTanks(skipDelay);
	EXPECT_EQ(_allObjects.size(), 0);

	_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::PLAYER1));

	_tankSpawner->RespawnTanks(skipDelay);
	EXPECT_EQ(_allObjects.size(), 1);
}

TEST_F(TankSpawnerTest, PlayerTwoDiedRespawn)
{
	constexpr bool skipDelay = true;
	_tankSpawner->RespawnTanks(skipDelay);
	EXPECT_EQ(_allObjects.size(), 0);

	_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::PLAYER2));

	_tankSpawner->RespawnTanks(skipDelay);
	EXPECT_EQ(_allObjects.size(), 1);
}

TEST_F(TankSpawnerTest, EnemyDiedRespawnCount)
{
	const int respawnCount = _tankSpawner->GetEnemyRespawnCount();//TODO: move to separated test group
	_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::ENEMY2));
	constexpr bool skipDelay = true;
	_tankSpawner->RespawnTanks(skipDelay);
	_allObjects.pop_back();

	EXPECT_GT(respawnCount, _tankSpawner->GetEnemyRespawnCount());
}

TEST_F(TankSpawnerTest, PlayerOneDiedRespawnCount)
{
	const int respawnCount = _tankSpawner->GetPlayerOneRespawnCount();
	_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::PLAYER1));
	constexpr bool skipDelay = true;
	_tankSpawner->RespawnTanks(skipDelay);
	_allObjects.pop_back();

	EXPECT_GT(respawnCount, _tankSpawner->GetPlayerOneRespawnCount());
}

TEST_F(TankSpawnerTest, PlayerTwoDiedRespawnCount)
{
	const int respawnCount = _tankSpawner->GetPlayerTwoRespawnCount();
	_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::PLAYER2));
	constexpr bool skipDelay = true;
	_tankSpawner->RespawnTanks(skipDelay);
	_allObjects.pop_back();

	EXPECT_GT(respawnCount, _tankSpawner->GetPlayerTwoRespawnCount());
}

TEST_F(TankSpawnerTest, EnemyRunOutRespawnPoints)
{
	const int respawnCount = _tankSpawner->GetEnemyRespawnCount();
	_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::ENEMY2));

	for (int i = 0; i < 20; ++i)
	{
		constexpr bool skipDelay = true;
		_tankSpawner->RespawnTanks(skipDelay);
		_allObjects.pop_back();
	}

	EXPECT_GT(respawnCount, _tankSpawner->GetEnemyRespawnCount());
	EXPECT_EQ(0, _tankSpawner->GetEnemyRespawnCount());
}

TEST_F(TankSpawnerTest, PlayerOneRunOutRespawnPoints)
{
	const int respawnCount = _tankSpawner->GetPlayerOneRespawnCount();
	_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::PLAYER1));

	for (int i = 0; i < 3; ++i)
	{
		constexpr bool skipDelay = true;
		_tankSpawner->RespawnTanks(skipDelay);
		_allObjects.pop_back();
	}

	EXPECT_GT(respawnCount, _tankSpawner->GetPlayerOneRespawnCount());
	EXPECT_EQ(0, _tankSpawner->GetPlayerOneRespawnCount());
}

TEST_F(TankSpawnerTest, PlayerTwoRunOutRespawnPoints)
{
	const int respawnCount = _tankSpawner->GetPlayerTwoRespawnCount();
	_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::PLAYER2));

	for (int i = 0; i < 3; ++i)
	{
		constexpr bool skipDelay = true;
		_tankSpawner->RespawnTanks(skipDelay);
		_allObjects.pop_back();
	}

	EXPECT_GT(respawnCount, _tankSpawner->GetPlayerTwoRespawnCount());
	EXPECT_EQ(0, _tankSpawner->GetPlayerTwoRespawnCount());
}

TEST_F(TankSpawnerTest, EnemyRunOutRespawnPointsAndTryMore)
{
	const int respawnCount = _tankSpawner->GetEnemyRespawnCount();
	_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::ENEMY2));

	for (int i = 0; i < 21; ++i)
	{
		constexpr bool skipDelay = true;
		_tankSpawner->RespawnTanks(skipDelay);
		if (!_allObjects.empty())
		{
			_allObjects.pop_back();
		}
	}

	EXPECT_GT(respawnCount, _tankSpawner->GetEnemyRespawnCount());
	EXPECT_EQ(0, _tankSpawner->GetEnemyRespawnCount());
}

TEST_F(TankSpawnerTest, PlayerOneRunOutRespawnPointsAndTryMore)
{
	const int respawnCount = _tankSpawner->GetPlayerOneRespawnCount();
	_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::PLAYER1));

	for (int i = 0; i < 4; ++i)
	{
		constexpr bool skipDelay = true;
		_tankSpawner->RespawnTanks(skipDelay);
		if (!_allObjects.empty())
		{
			_allObjects.pop_back();
		}
	}

	EXPECT_GT(respawnCount, _tankSpawner->GetPlayerOneRespawnCount());
	EXPECT_EQ(0, _tankSpawner->GetPlayerOneRespawnCount());
}

TEST_F(TankSpawnerTest, PlayerTwoRunOutRespawnPointsAndTryMore)
{
	const int respawnCount = _tankSpawner->GetPlayerTwoRespawnCount();
	_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::PLAYER2));

	for (int i = 0; i < 4; ++i)
	{
		constexpr bool skipDelay = true;
		_tankSpawner->RespawnTanks(skipDelay);
		if (!_allObjects.empty())
		{
			_allObjects.pop_back();
		}
	}

	EXPECT_GT(respawnCount, _tankSpawner->GetPlayerTwoRespawnCount());
	EXPECT_EQ(0, _tankSpawner->GetPlayerTwoRespawnCount());
}
