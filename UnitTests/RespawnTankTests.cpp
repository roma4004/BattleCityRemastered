#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/TankSpawner.h"
#include "components/managers/BonusEffectManager.h"
#include "components/managers/SpawnDelayManager.h"
#include "enums/GameMode.h"
#include "enums/TankType.h"
#include "gtest/gtest.h"
#include <memory>

class TankSpawnerTest : public testing::Test
{
protected:
	std::shared_ptr<RespawnResourceManager> _respawnResourceManager{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<SpawnDelayManager> _spawnDelayManager{nullptr};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;

	void SetUp() override
	{
		_allObjects.reserve(6);
		constexpr UPoint windowSize{.x = 800, .y = 600};
		const auto events = std::make_shared<EventSystem>();
		const auto bulletPool = std::make_shared<BulletPool>(events, &_allObjects, windowSize, GameMode::OnePlayer);
		const auto effectsManager = std::make_shared<BonusEffectManager>(events);
		_respawnResourceManager = std::make_shared<RespawnResourceManager>(events);
		_tankSpawner = std::make_shared<TankSpawner>(
				windowSize, &_allObjects, events, bulletPool, effectsManager, _respawnResourceManager);
		_spawnDelayManager = std::make_shared<SpawnDelayManager>(events);
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

	_respawnResourceManager->SetSlotNeedRespawn(static_cast<int>(TankType::ENEMY1));

	_tankSpawner->RespawnTanks(skipDelay);
	EXPECT_EQ(_allObjects.size(), 1);
}

TEST_F(TankSpawnerTest, EnemyTwoRespawn)
{
	constexpr bool skipDelay = true;
	_tankSpawner->RespawnTanks(skipDelay);
	EXPECT_EQ(_allObjects.size(), 0);

	_respawnResourceManager->SetSlotNeedRespawn(static_cast<int>(TankType::ENEMY2));

	_tankSpawner->RespawnTanks(skipDelay);
	EXPECT_EQ(_allObjects.size(), 1);
}

TEST_F(TankSpawnerTest, EnemyThreeRespawn)
{
	constexpr bool skipDelay = true;
	_tankSpawner->RespawnTanks(skipDelay);
	EXPECT_EQ(_allObjects.size(), 0);

	_respawnResourceManager->SetSlotNeedRespawn(static_cast<int>(TankType::ENEMY3));

	_tankSpawner->RespawnTanks(skipDelay);
	EXPECT_EQ(_allObjects.size(), 1);
}

TEST_F(TankSpawnerTest, EnemyFourRespawn)
{
	constexpr bool skipDelay = true;
	_tankSpawner->RespawnTanks(skipDelay);
	EXPECT_EQ(_allObjects.size(), 0);

	_respawnResourceManager->SetSlotNeedRespawn(static_cast<int>(TankType::ENEMY4));

	_tankSpawner->RespawnTanks(skipDelay);
	EXPECT_EQ(_allObjects.size(), 1);
}

TEST_F(TankSpawnerTest, PlayerOneDiedRespawn)
{
	constexpr bool skipDelay = true;
	_tankSpawner->RespawnTanks(skipDelay);
	EXPECT_EQ(_allObjects.size(), 0);

	_respawnResourceManager->SetSlotNeedRespawn(static_cast<int>(TankType::PLAYER1));

	_tankSpawner->RespawnTanks(skipDelay);
	EXPECT_EQ(_allObjects.size(), 1);
}

TEST_F(TankSpawnerTest, PlayerTwoDiedRespawn)
{
	constexpr bool skipDelay = true;
	_tankSpawner->RespawnTanks(skipDelay);
	EXPECT_EQ(_allObjects.size(), 0);

	_respawnResourceManager->SetSlotNeedRespawn(static_cast<int>(TankType::PLAYER2));

	_tankSpawner->RespawnTanks(skipDelay);
	EXPECT_EQ(_allObjects.size(), 1);
}

TEST_F(TankSpawnerTest, EnemyDiedRespawnCount)
{
	const int respawnResource = _respawnResourceManager->GetEnemyRespawnResource();//TODO: move to separated test group
	_respawnResourceManager->SetSlotNeedRespawn(static_cast<int>(TankType::ENEMY2));
	constexpr bool skipDelay = true;
	_tankSpawner->RespawnTanks(skipDelay);
	_allObjects.pop_back();

	EXPECT_GT(respawnResource, _respawnResourceManager->GetEnemyRespawnResource());
}

TEST_F(TankSpawnerTest, PlayerOneDiedRespawnCount)
{
	const int respawnResource = _respawnResourceManager->GetPlayerOneRespawnResource();
	_respawnResourceManager->SetSlotNeedRespawn(static_cast<int>(TankType::PLAYER1));
	constexpr bool skipDelay = true;
	_tankSpawner->RespawnTanks(skipDelay);
	_allObjects.pop_back();

	EXPECT_GT(respawnResource, _respawnResourceManager->GetPlayerOneRespawnResource());
}

TEST_F(TankSpawnerTest, PlayerTwoDiedRespawnCount)
{
	const int respawnResource = _respawnResourceManager->GetPlayerTwoRespawnResource();
	_respawnResourceManager->SetSlotNeedRespawn(static_cast<int>(TankType::PLAYER2));
	constexpr bool skipDelay = true;
	_tankSpawner->RespawnTanks(skipDelay);
	_allObjects.pop_back();

	EXPECT_GT(respawnResource, _respawnResourceManager->GetPlayerTwoRespawnResource());
}

TEST_F(TankSpawnerTest, EnemyRunOutRespawnPoints)
{
	const int respawnResource = _respawnResourceManager->GetEnemyRespawnResource();
	_respawnResourceManager->SetSlotNeedRespawn(static_cast<int>(TankType::ENEMY2));

	for (int i = 0; i < 20; ++i)
	{
		constexpr bool skipDelay = true;
		_tankSpawner->RespawnTanks(skipDelay);
		_allObjects.pop_back();
	}

	EXPECT_GT(respawnResource, _respawnResourceManager->GetEnemyRespawnResource());
	EXPECT_EQ(0, _respawnResourceManager->GetEnemyRespawnResource());
}

TEST_F(TankSpawnerTest, PlayerOneRunOutRespawnPoints)
{
	const int respawnResource = _respawnResourceManager->GetPlayerOneRespawnResource();
	_respawnResourceManager->SetSlotNeedRespawn(static_cast<int>(TankType::PLAYER1));

	for (int i = 0; i < 3; ++i)
	{
		constexpr bool skipDelay = true;
		_tankSpawner->RespawnTanks(skipDelay);
		_allObjects.pop_back();
	}

	EXPECT_GT(respawnResource, _respawnResourceManager->GetPlayerOneRespawnResource());
	EXPECT_EQ(0, _respawnResourceManager->GetPlayerOneRespawnResource());
}

TEST_F(TankSpawnerTest, PlayerTwoRunOutRespawnPoints)
{
	const int respawnResource = _respawnResourceManager->GetPlayerTwoRespawnResource();
	_respawnResourceManager->SetSlotNeedRespawn(static_cast<int>(TankType::PLAYER2));

	for (int i = 0; i < 3; ++i)
	{
		constexpr bool skipDelay = true;
		_tankSpawner->RespawnTanks(skipDelay);
		_allObjects.pop_back();
	}

	EXPECT_GT(respawnResource, _respawnResourceManager->GetPlayerTwoRespawnResource());
	EXPECT_EQ(0, _respawnResourceManager->GetPlayerTwoRespawnResource());
}

TEST_F(TankSpawnerTest, EnemyRunOutRespawnPointsAndTryMore)
{
	const int respawnResource = _respawnResourceManager->GetEnemyRespawnResource();
	_respawnResourceManager->SetSlotNeedRespawn(static_cast<int>(TankType::ENEMY2));

	for (int i = 0; i < 21; ++i)
	{
		constexpr bool skipDelay = true;
		_tankSpawner->RespawnTanks(skipDelay);
		if (!_allObjects.empty())
		{
			_allObjects.pop_back();
		}
	}

	EXPECT_GT(respawnResource, _respawnResourceManager->GetEnemyRespawnResource());
	EXPECT_EQ(0, _respawnResourceManager->GetEnemyRespawnResource());
}

TEST_F(TankSpawnerTest, PlayerOneRunOutRespawnPointsAndTryMore)
{
	const int respawnResource = _respawnResourceManager->GetPlayerOneRespawnResource();
	_respawnResourceManager->SetSlotNeedRespawn(static_cast<int>(TankType::PLAYER1));

	for (int i = 0; i < 4; ++i)
	{
		constexpr bool skipDelay = true;
		_tankSpawner->RespawnTanks(skipDelay);
		if (!_allObjects.empty())
		{
			_allObjects.pop_back();
		}
	}

	EXPECT_GT(respawnResource, _respawnResourceManager->GetPlayerOneRespawnResource());
	EXPECT_EQ(0, _respawnResourceManager->GetPlayerOneRespawnResource());
}

TEST_F(TankSpawnerTest, PlayerTwoRunOutRespawnPointsAndTryMore)
{
	const int respawnResource = _respawnResourceManager->GetPlayerTwoRespawnResource();
	_respawnResourceManager->SetSlotNeedRespawn(static_cast<int>(TankType::PLAYER2));

	for (int i = 0; i < 4; ++i)
	{
		constexpr bool skipDelay = true;
		_tankSpawner->RespawnTanks(skipDelay);
		if (!_allObjects.empty())
		{
			_allObjects.pop_back();
		}
	}

	EXPECT_GT(respawnResource, _respawnResourceManager->GetPlayerTwoRespawnResource());
	EXPECT_EQ(0, _respawnResourceManager->GetPlayerTwoRespawnResource());
}
