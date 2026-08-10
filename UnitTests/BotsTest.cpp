#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/TankSpawner.h"
#include "components/managers/DelayedSpawnManager.h"
#include "components/managers/RespawnManager.h"
#include "components/managers/GameStateManager.h"
#include "entities/obstacles/BushTile.h"
#include "entities/obstacles/IceTile.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/pawns/CoopBot.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/Player.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include <memory>

class BotsTest : public testing::Test// NOLINT(clang-diagnostic-padded)
{
	using buuid = boost::uuids::uuid;

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::unique_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<GameStateManager> _stateManager{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<RespawnManager> _respawnManager{nullptr};
	std::shared_ptr<DelayedSpawnManager> _spawnDelayManager{nullptr};
	GameConfig _gameConfig{"", true};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.f / 60.f};
	buuid _uuid{};
	float _tankSize{};
	float _tankSpeed{142};
	float _gridSize{};
	unsigned short _tankHealth{100u};
	GameMode _gameMode{GameMode::OnePlayer};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		TestUtils::WireSpawnQueue(_events, &_allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _gameConfig);
		_stateManager = std::make_shared<GameStateManager>(_events);
		_respawnManager = std::make_shared<RespawnManager>(_events);
		_tankSpawner = std::make_shared<TankSpawner>(_gameConfig, &_allObjects, _events, *_respawnManager);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, &_allObjects, _gameConfig);
		_spawnDelayManager = std::make_shared<DelayedSpawnManager>(_events);
		_gridSize = static_cast<float>(_gameConfig.windowSize.y) / 50.f;
		_tankSize = _gridSize * 3;// for better turns

		_allObjects.reserve(4u);
	}

	void TearDown() override
	{
		_events->RemoveListener("AddToSpawnQueue", "TestSpawnQueue");
	}
};

// Check that bots change direction to the opponent
TEST_F(BotsTest, BotsChangeDirectionIfOpponentSeen)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<CoopBot> coopBot =
			TestUtils::CreateTank<CoopBot>(
					coopBotRect, _tankHealth, _uuid, "CoopBot1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 3.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const Direction startDirCoop = coopBot->GetDirection();
	const Direction startDirEnemy = enemyBot->GetDirection();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const Direction endDirCoop = coopBot->GetDirection();
	const Direction endDirEnemy = enemyBot->GetDirection();

	EXPECT_NE(startDirCoop, endDirCoop);
	EXPECT_NE(startDirEnemy, endDirEnemy);
	EXPECT_EQ(endDirCoop, Direction::RIGHT);
	EXPECT_EQ(endDirEnemy, Direction::LEFT);
}

/*// Check that bots not change direction to the opponent if too close
TEST_F(BotsTest, BotsNoChangeDirectionIfOpponentSeen)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<CoopBot> coopBot =
			TestUtils::CreateTank<CoopBot>(
					coopBotRect, _tankHealth, _uuid, "CoopBot1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize /*+ 1.f#1#, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const Direction startDirCoop = coopBot->GetDirection();
	const Direction startDirEnemy = enemyBot->GetDirection();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const Direction endDirCoop = coopBot->GetDirection();
	const Direction endDirEnemy = enemyBot->GetDirection();

	EXPECT_EQ(startDirCoop, endDirCoop);
	EXPECT_EQ(startDirEnemy, endDirEnemy);
	EXPECT_EQ(endDirCoop, Direction::DOWN);
	EXPECT_EQ(endDirEnemy, Direction::DOWN);
}*/

// Check that bots change their direction to the bonus
TEST_F(BotsTest, BotsChangeDirectionIfBonusSeenAndNoOneShoot)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<CoopBot> coopBot =
			TestUtils::CreateTank<CoopBot>(
					coopBotRect, _tankHealth, _uuid, "CoopBot1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	_bonusSpawner->SpawnRandomBonus({.x = _tankSize + 21.f, .y = 0.f, .w = _tankSize, .h = _tankSize});

	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 3.f + 40.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const Direction startDirCoop = coopBot->GetDirection();
	const Direction startDirEnemy = enemyBot->GetDirection();
	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const Direction endDirCoop = coopBot->GetDirection();
	const Direction endDirEnemy = enemyBot->GetDirection();

	EXPECT_EQ(sizeBefore, _allObjects.size());
	EXPECT_NE(startDirCoop, endDirCoop);
	EXPECT_NE(startDirEnemy, endDirEnemy);
	EXPECT_EQ(endDirCoop, Direction::RIGHT);
	EXPECT_EQ(endDirEnemy, Direction::LEFT);
}

// Check that both Bots can't see bonus that behind water
TEST_F(BotsTest, BotsCantSeeBonusBehindWater)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<CoopBot> coopBot =
			TestUtils::CreateTank<CoopBot>(
					coopBotRect, _tankHealth, _uuid, "CoopBot1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::RIGHT, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Water
	_allObjects.emplace_back(
			std::make_shared<WaterTile>(
					ObjRectangle{.x = 0.f, .y = _tankSize * 2.f + 1.f, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameMode));

	// Spawn Bonus
	_bonusSpawner->SpawnRandomBonus({.x = 0.f, .y = _tankSize * 3.f + 2.f, .w = _tankSize, .h = _tankSize});

	// Spawn Water
	_allObjects.emplace_back(
			std::make_shared<WaterTile>(
					ObjRectangle{.x = 0.f, .y = _tankSize * 4.f + 3.f, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameMode));

	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = _tankSize * 5.f + 40.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::RIGHT, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const Direction startDirCoop = coopBot->GetDirection();
	const Direction startDirEnemy = enemyBot->GetDirection();
	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const Direction endDirCoop = coopBot->GetDirection();
	const Direction endDirEnemy = enemyBot->GetDirection();

	EXPECT_EQ(sizeBefore, _allObjects.size());
	EXPECT_EQ(startDirCoop, endDirCoop);
	EXPECT_EQ(startDirEnemy, endDirEnemy);
	EXPECT_EQ(endDirCoop, Direction::RIGHT);
	EXPECT_EQ(endDirEnemy, Direction::RIGHT);
}

// Check that both Bots can't see bonus that behind Bush
TEST_F(BotsTest, BotsCantSeeBonusBehindBush)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<CoopBot> coopBot =
			TestUtils::CreateTank<CoopBot>(
					coopBotRect, _tankHealth, _uuid, "CoopBot1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::RIGHT, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Bush
	_allObjects.emplace_back(
			std::make_shared<BushTile>(
					ObjRectangle{.x = 0.f, .y = _tankSize * 2.f + 1.f, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameMode));

	// Spawn Bonus
	_bonusSpawner->SpawnRandomBonus({.x = 0.f, .y = _tankSize * 3.f + 2.f, .w = _tankSize, .h = _tankSize});

	// Spawn Bush
	_allObjects.emplace_back(
			std::make_shared<BushTile>(
					ObjRectangle{.x = 0.f, .y = _tankSize * 4.f + 3.f, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameMode));

	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = _tankSize * 5.f + 40.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::RIGHT, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const Direction startDirCoop = coopBot->GetDirection();
	const Direction startDirEnemy = enemyBot->GetDirection();
	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const Direction endDirCoop = coopBot->GetDirection();
	const Direction endDirEnemy = enemyBot->GetDirection();

	EXPECT_EQ(sizeBefore, _allObjects.size());
	EXPECT_EQ(startDirCoop, endDirCoop);
	EXPECT_EQ(startDirEnemy, endDirEnemy);
	EXPECT_EQ(endDirCoop, Direction::RIGHT);
	EXPECT_EQ(endDirEnemy, Direction::RIGHT);
}

// Check that both Bots can see bonus that behind Ice
TEST_F(BotsTest, BotsCanSeeBonusBehindIce)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<CoopBot> coopBot =
			TestUtils::CreateTank<CoopBot>(
					coopBotRect, _tankHealth, _uuid, "CoopBot1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::RIGHT, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Ice
	_allObjects.emplace_back(
			std::make_shared<IceTile>(
					ObjRectangle{.x = 0.f, .y = _tankSize * 2.f + 1.f, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameMode));

	// Spawn Bonus
	_bonusSpawner->SpawnRandomBonus({.x = 0.f, .y = _tankSize * 3.f + 2.f, .w = _tankSize, .h = _tankSize});

	// Spawn Ice
	_allObjects.emplace_back(
			std::make_shared<IceTile>(
					ObjRectangle{.x = 0.f, .y = _tankSize * 4.f + 3.f, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameMode));

	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = _tankSize * 5.f + 40.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::RIGHT, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const Direction startDirCoop = coopBot->GetDirection();
	const Direction startDirEnemy = enemyBot->GetDirection();
	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const Direction endDirCoop = coopBot->GetDirection();
	const Direction endDirEnemy = enemyBot->GetDirection();

	EXPECT_EQ(sizeBefore, _allObjects.size());
	EXPECT_NE(startDirCoop, endDirCoop);
	EXPECT_NE(startDirEnemy, endDirEnemy);
	EXPECT_EQ(endDirCoop, Direction::DOWN);
	EXPECT_EQ(endDirEnemy, Direction::UP);
}

// Check that both Bots can see bonus that in the Ice
TEST_F(BotsTest, BotsCanSeeBonusInTheIce)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<CoopBot> coopBot =
			TestUtils::CreateTank<CoopBot>(
					coopBotRect, _tankHealth, _uuid, "CoopBot1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::RIGHT, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Ice
	_allObjects.emplace_back(
			std::make_shared<IceTile>(
					ObjRectangle{.x = 0.f, .y = _tankSize * 2.f + 1.f, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameMode));

	// Spawn Ice
	_allObjects.emplace_back(
			std::make_shared<IceTile>(
					ObjRectangle{.x = 0.f, .y = _tankSize * 3.f + 1.f, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameMode));

	// Spawn Bonus
	_bonusSpawner->SpawnRandomBonus({.x = 0.f, .y = _tankSize * 3.f + 2.f, .w = _tankSize, .h = _tankSize});

	// Spawn Ice
	_allObjects.emplace_back(
			std::make_shared<IceTile>(
					ObjRectangle{.x = 0.f, .y = _tankSize * 4.f + 3.f, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameMode));

	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = _tankSize * 5.f + 40.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::RIGHT, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const Direction startDirCoop = coopBot->GetDirection();
	const Direction startDirEnemy = enemyBot->GetDirection();
	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const Direction endDirCoop = coopBot->GetDirection();
	const Direction endDirEnemy = enemyBot->GetDirection();

	EXPECT_EQ(sizeBefore, _allObjects.size());
	EXPECT_NE(startDirCoop, endDirCoop);
	EXPECT_NE(startDirEnemy, endDirEnemy);
	EXPECT_EQ(endDirCoop, Direction::DOWN);
	EXPECT_EQ(endDirEnemy, Direction::UP);
}
