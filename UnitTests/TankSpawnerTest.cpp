#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/TankSpawner.h"
#include "components/managers/DelayedSpawnManager.h"
#include "enums/TankType.h"
#include "gtest/gtest.h"
#include <memory>

class TankSpawnerTest : public testing::Test
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
		_allObjects.reserve(6);
		const auto bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _gameConfig);
		_tankSpawner = std::make_shared<TankSpawner>(_gameConfig, &_allObjects, _events);
		_spawnDelayManager = std::make_shared<DelayedSpawnManager>(_events);
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

TEST_F(TankSpawnerTest, EnemyOneRespawn)
{
	constexpr bool skipDelay{true};
	_events->EmitEvent("RespawnTanks", skipDelay);
	EXPECT_EQ(_allObjects.size(), 0u);

	_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::ENEMY1));

	_events->EmitEvent("RespawnTanks", skipDelay);
	EXPECT_EQ(_allObjects.size(), 1u);
}

TEST_F(TankSpawnerTest, EnemyTwoRespawn)
{
	constexpr bool skipDelay{true};
	_events->EmitEvent("RespawnTanks", skipDelay);
	EXPECT_EQ(_allObjects.size(), 0u);

	_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::ENEMY2));

	_events->EmitEvent("RespawnTanks", skipDelay);
	EXPECT_EQ(_allObjects.size(), 1u);
}

TEST_F(TankSpawnerTest, EnemyThreeRespawn)
{
	constexpr bool skipDelay{true};
	_events->EmitEvent("RespawnTanks", skipDelay);
	EXPECT_EQ(_allObjects.size(), 0u);

	_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::ENEMY3));

	_events->EmitEvent("RespawnTanks", skipDelay);
	EXPECT_EQ(_allObjects.size(), 1u);
}

TEST_F(TankSpawnerTest, EnemyFourRespawn)
{
	constexpr bool skipDelay{true};
	_events->EmitEvent("RespawnTanks", skipDelay);
	EXPECT_EQ(_allObjects.size(), 0u);

	_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::ENEMY4));

	_events->EmitEvent("RespawnTanks", skipDelay);
	EXPECT_EQ(_allObjects.size(), 1u);
}

TEST_F(TankSpawnerTest, PlayerOneDiedRespawn)
{
	constexpr bool skipDelay{true};
	_events->EmitEvent("RespawnTanks", skipDelay);
	EXPECT_EQ(_allObjects.size(), 0u);

	_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::PLAYER1));

	_events->EmitEvent("RespawnTanks", skipDelay);
	EXPECT_EQ(_allObjects.size(), 1u);
}

TEST_F(TankSpawnerTest, PlayerTwoDiedRespawn)
{
	constexpr bool skipDelay{true};
	_events->EmitEvent("RespawnTanks", skipDelay);
	EXPECT_EQ(_allObjects.size(), 0u);

	_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::PLAYER2));

	_events->EmitEvent("RespawnTanks", skipDelay);
	EXPECT_EQ(_allObjects.size(), 1u);
}
