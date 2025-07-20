#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/TankSpawner.h"
#include "components/managers/BonusEffectManager.h"
#include "enums/GameMode.h"
#include "enums/TankType.h"
#include "gtest/gtest.h"
#include <memory>

class TankSpawnerTest : public testing::Test
{
protected:
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;

	void SetUp() override
	{
		constexpr UPoint windowSize{.x = 800, .y = 600};
		const auto events = std::make_shared<EventSystem>();
		const auto bulletPool = std::make_shared<BulletPool>(events, &_allObjects, windowSize, GameMode::OnePlayer);
		const auto effectsManager = std::make_shared<BonusEffectManager>(events);
		_tankSpawner = std::make_shared<TankSpawner>(windowSize, &_allObjects, events, bulletPool, effectsManager);
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

TEST_F(TankSpawnerTest, EnemyOneRespawn)
{
	_tankSpawner->RespawnTanks();
	EXPECT_EQ(_allObjects.size(), 0);

	_tankSpawner->SetSlotNeedRespawn(static_cast<int>(TankType::ENEMY1));

	_tankSpawner->RespawnTanks();
	EXPECT_EQ(_allObjects.size(), 1);
}

TEST_F(TankSpawnerTest, EnemyTwoRespawn)
{
	_tankSpawner->RespawnTanks();
	EXPECT_EQ(_allObjects.size(), 0);

	_tankSpawner->SetSlotNeedRespawn(static_cast<int>(TankType::ENEMY2));

	_tankSpawner->RespawnTanks();
	EXPECT_EQ(_allObjects.size(), 1);
}

TEST_F(TankSpawnerTest, EnemyThreeRespawn)
{
	_tankSpawner->RespawnTanks();
	EXPECT_EQ(_allObjects.size(), 0);

	_tankSpawner->SetSlotNeedRespawn(static_cast<int>(TankType::ENEMY3));

	_tankSpawner->RespawnTanks();
	EXPECT_EQ(_allObjects.size(), 1);
}

TEST_F(TankSpawnerTest, EnemyFourRespawn)
{
	_tankSpawner->RespawnTanks();
	EXPECT_EQ(_allObjects.size(), 0);

	_tankSpawner->SetSlotNeedRespawn(static_cast<int>(TankType::ENEMY4));

	_tankSpawner->RespawnTanks();
	EXPECT_EQ(_allObjects.size(), 1);
}

TEST_F(TankSpawnerTest, PlayerOneDiedRespawn)
{
	_tankSpawner->RespawnTanks();
	EXPECT_EQ(_allObjects.size(), 0);

	_tankSpawner->SetSlotNeedRespawn(static_cast<int>(TankType::PLAYER1));

	_tankSpawner->RespawnTanks();
	EXPECT_EQ(_allObjects.size(), 1);
}

TEST_F(TankSpawnerTest, PlayerTwoDiedRespawn)
{
	_tankSpawner->RespawnTanks();
	EXPECT_EQ(_allObjects.size(), 0);

	_tankSpawner->SetSlotNeedRespawn(static_cast<int>(TankType::PLAYER2));

	_tankSpawner->RespawnTanks();
	EXPECT_EQ(_allObjects.size(), 1);
}

TEST_F(TankSpawnerTest, EnemyDiedRespawnCount)
{
	const int respawnResource = _tankSpawner->GetEnemyRespawnResource();
	_tankSpawner->SetSlotNeedRespawn(static_cast<int>(TankType::ENEMY2));
	_tankSpawner->RespawnTanks();
	_allObjects.pop_back();

	EXPECT_GT(respawnResource, _tankSpawner->GetEnemyRespawnResource());
}

TEST_F(TankSpawnerTest, PlayerOneDiedRespawnCount)
{
	const int respawnResource = _tankSpawner->GetPlayerOneRespawnResource();
	_tankSpawner->SetSlotNeedRespawn(static_cast<int>(TankType::PLAYER1));
	_tankSpawner->RespawnTanks();
	_allObjects.pop_back();

	EXPECT_GT(respawnResource, _tankSpawner->GetPlayerOneRespawnResource());
}

TEST_F(TankSpawnerTest, PlayerTwoDiedRespawnCount)
{
	const int respawnResource = _tankSpawner->GetPlayerTwoRespawnResource();
	_tankSpawner->SetSlotNeedRespawn(static_cast<int>(TankType::PLAYER2));
	_tankSpawner->RespawnTanks();
	_allObjects.pop_back();

	EXPECT_GT(respawnResource, _tankSpawner->GetPlayerTwoRespawnResource());
}
