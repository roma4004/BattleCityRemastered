#include "TestUtils.h"
#include "components/ObstacleSpawner.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/managers/BonusManager.h"
#include "components/BulletPool.h"
#include "components/TankPool.h"
#include "components/EventSystem.h"
#include "components/events/InputEvents.h"
#include "components/events/TimingEvents.h"
#include "components/GameStatistics.h"
#include "components/StatisticsData.h"
#include "entities/bonuses/Bonus.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "enums/InputChannel.h"
#include "geometry/Point.h"
#include "gtest/gtest.h"
#include <chrono>
#include <memory>
#include <thread>

using namespace std::chrono_literals;

class StatisticsTest : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::shared_ptr<GameStatistics> _statistics{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<TankPool> _tankPool{nullptr};
	std::shared_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<BonusManager> _bonusManager{nullptr};
	std::vector<EventSubscription> _instantSpawnAnimationSubs{};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.0 / 60.0};
	BulletCalibre _calibre{.speed = 300.0, .damage = 1u, .damageRadius = 12.0, .tier = 1u, .size{.x = 6.0, .y = 5.0}};
	double _tankSize{};
	Uuid _uuid{};
	unsigned short _tankHealth{1u};
	unsigned short _bulletHealth{1u};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_obstacleSpawner = std::make_unique<ObstacleSpawner>(_events, _gameConfig);
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_tankPool = std::make_shared<TankPool>(_events, _allObjects, _gameConfig, _bulletPool);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, _allObjects, _gameConfig);
		_bonusManager = std::make_shared<BonusManager>(_events, _gameConfig);
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
		_statistics = std::make_shared<GameStatistics>(_events);
		_tankSize = _gameConfig.tankSize;

		_allObjects.reserve(5);
	}

	void TearDown() override {}

	std::shared_ptr<Tank> CreatePlayer(const FPoint pos, const Author author = Author::Player1,
									   const Direction dir = Direction::UP)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};
		auto player{TestUtils::CreatePlayer(rect, _tankHealth, author, _allObjects, _events, dir, _tankPool,
											_gameConfig)};

		return player;
	}

	std::shared_ptr<Tank> CreateBot(const FPoint pos, const Author author, const Direction dir,
									const unsigned short tier = 1u)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};
		auto bot{TestUtils::CreateBot(rect, _tankHealth, author, _allObjects, _events, dir, _tankPool, _gameConfig,
									  tier)};

		return bot;
	}

	std::shared_ptr<Bullet> CreateBullet(const FPoint pos, const Direction dir, const Author author)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _calibre.size.x, .h = _calibre.size.y};
		auto bullet{TestUtils::CreateBullet(rect, _bulletHealth, _bulletPool, _events, _calibre, dir,
											author)};

		return bullet;
	}

	void SpawnObstacleArea(const ObjRectangle area, const ObstacleType type) const
	{
		TestUtils::SpawnObstacleArea(_events, _allObjects, area, type, _gameConfig);
	}

	std::shared_ptr<BaseObj> SpawnObstacle(const FPoint pos, const ObstacleType type) const
	{
		return TestUtils::SpawnObstacle(_events, _allObjects, pos, type, _gameConfig);
	}
};

TEST_F(StatisticsTest, PlayerOneHitByEnemy)
{
	CreatePlayer({.x = 0.0, .y = 0.0});
	CreateBullet({.x = _tankSize / 2.0, .y = _tankSize}, Direction::UP, Author::Enemy1);

	EXPECT_EQ(_statistics->GetData().playerOneHitByEnemyTeam, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().playerOneHitByEnemyTeam, 1u);
}

TEST_F(StatisticsTest, PlayerOneHitByFriend)
{
	CreatePlayer({.x = 0.0, .y = 0.0});
	CreateBullet({.x = _tankSize / 2.0, .y = _tankSize + 1.0}, Direction::UP, Author::Player2);

	EXPECT_EQ(_statistics->GetData().playerOneHitFriendlyFire, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().playerOneHitFriendlyFire, 1u);
}

TEST_F(StatisticsTest, PlayerTwoHitByEnemy)
{
	CreatePlayer({.x = _tankSize + 1.0, .y = 0.0}, Author::Player2);
	CreateBullet({.x = _tankSize + _tankSize / 2.0, .y = _tankSize}, Direction::UP, Author::Enemy1);

	EXPECT_EQ(_statistics->GetData().playerTwoHitByEnemyTeam, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().playerTwoHitByEnemyTeam, 1u);
}

TEST_F(StatisticsTest, PlayerTwoHitByFriend)
{
	CreatePlayer({.x = _tankSize + 1.0, .y = 0.0}, Author::Player2);
	CreateBullet({.x = _tankSize + _tankSize / 2.0, .y = _tankSize}, Direction::UP, Author::Player1);

	EXPECT_EQ(_statistics->GetData().playerTwoHitFriendlyFire, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().playerTwoHitFriendlyFire, 1u);
}

TEST_F(StatisticsTest, PlayerOneDiedByFriend)
{
	CreatePlayer({.x = 0.0, .y = 0.0});
	CreateBullet({.x = 0.0, .y = _tankSize}, Direction::UP, Author::Player2);

	EXPECT_EQ(_statistics->GetData().playerOneDiedByFriendlyFire, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().playerOneDiedByFriendlyFire, 1u);
}

TEST_F(StatisticsTest, PlayerTwoDiedByEnemy)
{
	CreatePlayer({.x = _tankSize + 1.0, .y = 0.0}, Author::Player2);
	CreateBullet({.x = _tankSize + _tankSize / 2.0, .y = _tankSize}, Direction::UP, Author::Enemy1);

	EXPECT_EQ(_statistics->GetData().playerDiedByEnemyTeam, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().playerDiedByEnemyTeam, 1u);
}

TEST_F(StatisticsTest, PlayerOneDiedByEnemy)
{
	CreatePlayer({.x = 0.0, .y = 0.0});
	CreateBullet({.x = _calibre.size.x, .y = _tankSize}, Direction::UP, Author::Enemy1);

	EXPECT_EQ(_statistics->GetData().playerDiedByEnemyTeam, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().playerDiedByEnemyTeam, 1u);
}

TEST_F(StatisticsTest, PlayerTwoDiedByFriend)
{
	CreatePlayer({.x = _tankSize + 1.0, .y = 0.0}, Author::Player2);
	CreateBullet({.x = _tankSize + _tankSize / 2.0, .y = _tankSize}, Direction::UP, Author::Player1);

	EXPECT_EQ(_statistics->GetData().playerTwoDiedByFriendlyFire, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().playerTwoDiedByFriendlyFire, 1u);
}

TEST_F(StatisticsTest, EnemyHitByFriend)
{
	CreateBot({.x = _tankSize * 2.0 + 2.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);
	CreateBullet({.x = _tankSize * 2.0 + 2.0 + _tankSize / 2.0, .y = _tankSize}, Direction::UP, Author::Enemy2);

	EXPECT_EQ(_statistics->GetData().enemyHitByFriendlyFire, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().enemyHitByFriendlyFire, 1u);
}

TEST_F(StatisticsTest, EnemyHitByPlayerOne)
{
	CreateBot({.x = _tankSize * 2.0 + 2.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);
	CreateBullet({.x = _tankSize * 2.0 + 2.0 + _tankSize / 2.0, .y = _tankSize}, Direction::UP, Author::Player1);

	EXPECT_EQ(_statistics->GetData().enemyHitByPlayerOne, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().enemyHitByPlayerOne, 1u);
}

TEST_F(StatisticsTest, EnemyHitByPlayerTwo)
{
	CreateBot({.x = _tankSize * 2.0 + 2.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);
	CreateBullet({.x = _tankSize * 2.0 + 2.0 + _tankSize / 2.0, .y = _tankSize + 1}, Direction::UP, Author::Player2);

	EXPECT_EQ(_statistics->GetData().enemyHitByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().enemyHitByPlayerTwo, 1u);
}

TEST_F(StatisticsTest, EnemyDiedByFriend)
{
	CreateBot({.x = _tankSize * 2.0 + 2.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);
	CreateBullet({.x = _tankSize * 2.0 + 2.0 + _tankSize / 2.0, .y = _tankSize}, Direction::UP, Author::Enemy2);

	EXPECT_EQ(_statistics->GetData().enemyDiedByFriendlyFire, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().enemyDiedByFriendlyFire, 1u);
}

TEST_F(StatisticsTest, EnemyDiedByPlayerOne)
{
	CreateBot({.x = _tankSize * 2.0 + 2.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);
	CreateBullet({.x = _tankSize * 2.0 + 2.0 + _tankSize / 2.0, .y = _tankSize + 1}, Direction::UP, Author::Player1);

	EXPECT_EQ(_statistics->GetData().enemyDiedByPlayerOne, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().enemyDiedByPlayerOne, 1u);
}

TEST_F(StatisticsTest, EnemyDiedByPlayerTwo)
{
	CreateBot({.x = _tankSize * 2.0 + 2.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);
	CreateBullet({.x = _tankSize * 2.0 + 2.0 + _tankSize / 2.0, .y = _tankSize}, Direction::UP, Author::Player2);

	EXPECT_EQ(_statistics->GetData().enemyDiedByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().enemyDiedByPlayerTwo, 1u);
}

TEST_F(StatisticsTest, BulletHitByPlayerTwo)
{
	CreateBullet({.x = 0.0, .y = _tankSize}, Direction::DOWN, Author::Player1);
	CreateBullet({.x = 0.0, .y = _tankSize + _calibre.size.y + 1.0}, Direction::UP, Author::Player2);

	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerOne, 1u);
	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerTwo, 1u);
}

// the counter is about bullets meeting bullets - a wall the shot destroys has a row of its own
TEST_F(StatisticsTest, BulletIntoBrickWallIsNotABulletHit)
{
	SpawnObstacle(FPoint{.x = 0.0, .y = _tankSize + _calibre.size.y + 1}, ObstacleType::Brick);
	CreateBullet({.x = 0.0, .y = _tankSize}, Direction::DOWN, Author::Player1);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().brickWallDiedByPlayerOne, 1u);
}

// a shot the wall swallows whole is nobody's hit either
TEST_F(StatisticsTest, BulletIntoSteelWallIsNotABulletHit)
{
	SpawnObstacle(FPoint{.x = 0.0, .y = _tankSize + _calibre.size.y + 1}, ObstacleType::Steel);
	CreateBullet({.x = 0.0, .y = _tankSize}, Direction::DOWN, Author::Player1);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().steelWallDiedByPlayerOne, 0u);
}

// and the shot that flies off the field is not one either - that was the bug
TEST_F(StatisticsTest, BulletIntoFieldEdgeIsNotABulletHit)
{
	CreateBullet({.x = 0.0, .y = 0.0}, Direction::UP, Author::Player1);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerOne, 0u);
}

TEST_F(StatisticsTest, BrickWallDiedByEnemy)
{
	SpawnObstacle(FPoint{.x = 0.0, .y = _tankSize + _calibre.size.y + 1}, ObstacleType::Brick);

	CreateBullet({.x = 0.0, .y = _tankSize}, Direction::DOWN, Author::Enemy1);

	EXPECT_EQ(_statistics->GetData().brickWallDiedByEnemyTeam, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().brickWallDiedByEnemyTeam, 1u);
}

TEST_F(StatisticsTest, BrickWallDiedByPlayerOne)
{
	SpawnObstacle(FPoint{.x = 0.0, .y = _tankSize + _calibre.size.y + 1}, ObstacleType::Brick);

	CreateBullet({.x = 0.0, .y = _tankSize}, Direction::DOWN, Author::Player1);

	EXPECT_EQ(_statistics->GetData().brickWallDiedByPlayerOne, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().brickWallDiedByPlayerOne, 1u);
}

TEST_F(StatisticsTest, BrickDiedByPlayerTwo)
{
	SpawnObstacle(FPoint{.x = 0.0, .y = _tankSize + _calibre.size.y + 1}, ObstacleType::Brick);

	CreateBullet({.x = 0.0, .y = _tankSize}, Direction::DOWN, Author::Player2);

	EXPECT_EQ(_statistics->GetData().brickWallDiedByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().brickWallDiedByPlayerTwo, 1u);
}

TEST_F(StatisticsTest, SteelWallDiedByEnemy)
{
	SpawnObstacle(FPoint{.x = 0.0, .y = _tankSize + _calibre.size.y + 1}, ObstacleType::Steel);

	_calibre.tier = 3u;
	CreateBullet({.x = 0.0, .y = _tankSize}, Direction::DOWN, Author::Enemy1);

	EXPECT_EQ(_statistics->GetData().steelWallDiedByEnemyTeam, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().steelWallDiedByEnemyTeam, 1u);
}

TEST_F(StatisticsTest, SteelWallDiedByPlayerOne)
{
	SpawnObstacle(FPoint{.x = 0.0, .y = _tankSize + _calibre.size.y + 1}, ObstacleType::Steel);

	_calibre.tier = 3u;
	CreateBullet({.x = 0.0, .y = _tankSize}, Direction::DOWN, Author::Player1);

	EXPECT_EQ(_statistics->GetData().steelWallDiedByPlayerOne, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().steelWallDiedByPlayerOne, 1u);
}

TEST_F(StatisticsTest, SteelDiedByPlayerTwo)
{
	SpawnObstacle(FPoint{.x = 0.0, .y = _tankSize + _calibre.size.y + 1}, ObstacleType::Steel);

	_calibre.tier = 3u;
	CreateBullet({.x = 0.0, .y = _tankSize}, Direction::DOWN, Author::Player2);

	EXPECT_EQ(_statistics->GetData().steelWallDiedByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().steelWallDiedByPlayerTwo, 1u);
}

TEST_F(StatisticsTest, BulletHitBulletByEnemyAndByEnemy)
{
	CreateBullet({.x = 0.0, .y = _tankSize}, Direction::DOWN, Author::Enemy1);
	CreateBullet({.x = 0.0, .y = _tankSize + _calibre.size.y + 1.0}, Direction::UP, Author::Enemy2);

	EXPECT_EQ(_statistics->GetData().bulletHitByEnemy, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bulletHitByEnemy, 2u);
}

TEST_F(StatisticsTest, BulletHitBulletPlayerOneAndByPlayerTwo)
{
	CreateBullet({.x = 0.0, .y = _tankSize}, Direction::DOWN, Author::Player1);
	CreateBullet({.x = 0.0, .y = _tankSize + _calibre.size.y + 1.0}, Direction::UP, Author::Player2);

	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerOne, 1u);
	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerTwo, 1u);
}

TEST_F(StatisticsTest, BulletHitBulletByEnemyAndByPlayerOne)
{
	CreateBullet({.x = 0.0, .y = _tankSize}, Direction::DOWN, Author::Player1);
	CreateBullet({.x = 0.0, .y = _tankSize + _calibre.size.y + 1.0}, Direction::UP, Author::Enemy1);

	EXPECT_EQ(_statistics->GetData().bulletHitByEnemy, 0u);
	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerOne, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bulletHitByEnemy, 1u);
	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerOne, 1u);
}

TEST_F(StatisticsTest, BulletHitBulletByEnemyAndByPlayerTwo)
{
	CreateBullet({.x = 0.0, .y = _tankSize}, Direction::DOWN, Author::Player2);
	CreateBullet({.x = 0.0, .y = _tankSize + _calibre.size.y + 1.0}, Direction::UP, Author::Enemy1);

	EXPECT_EQ(_statistics->GetData().bulletHitByEnemy, 0u);
	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bulletHitByEnemy, 1u);
	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerTwo, 1u);
}

TEST_F(StatisticsTest, BonusPickUpByEnemyCount)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize});
	_bonusSpawner->SpawnRandomBonus({.x = _tankSize + 1.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 1u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 0u);
}

TEST_F(StatisticsTest, BonusNotPickUpByEnemyNotCount)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize * 2 + 1.0, .w = _tankSize, .h = _tankSize});
	_bonusSpawner->SpawnRandomBonus({.x = _tankSize * 2 + 1.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 0u);
}

TEST_F(StatisticsTest, BonusPickUpByPlayerOneCount)
{
	CreatePlayer({.x = 0.0, .y = 0.0});

	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize});
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 1u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 0u);
}

TEST_F(StatisticsTest, BonusNotPickUpByPlayerOneNotCount)
{
	CreatePlayer({.x = 0.0, .y = 0.0});
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = isPressed});

	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 0u);
}

TEST_F(StatisticsTest, BonusPickUpByPlayerTwoCount)
{
	CreatePlayer({.x = _tankSize + 1.0, .y = 0.0}, Author::Player2);
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP2), MoveDownEvent{.isPressed = isPressed});

	_bonusSpawner->SpawnRandomBonus({.x = _tankSize + 1.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 1u);
}

TEST_F(StatisticsTest, BonusNotPickUpByPlayerTwoNotCount)
{
	CreatePlayer({.x = _tankSize + 1.0, .y = 0.0}, Author::Player2);
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP2), MoveUpEvent{.isPressed = isPressed});

	_bonusSpawner->SpawnRandomBonus({.x = _tankSize + 1.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 0u);
}

TEST_F(StatisticsTest, BonusExpiredCountedWithNoAuthor)
{
	//NOTE: how long a bonus lives is the spawner's, so the test shortens it instead of building one by hand
	_gameConfig.bonusLifeTimeCooldown = 1ms;
	_bonusSpawner->SpawnBonus({.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize}, BonusType::Helmet);
	ASSERT_EQ(_allObjects.size(), 1u);

	EXPECT_EQ(_statistics->GetData().bonusExpired, 0u);

	std::this_thread::sleep_for(2ms);
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bonusExpired, 1u);
	EXPECT_FALSE(_allObjects.back()->GetIsAlive());

	//NOTE: the timer is one-shot - a second tick must not keep counting the same bonus
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bonusExpired, 1u);
}

TEST_F(StatisticsTest, BonusShotIsCountedAndPickupIsNot)
{
	const ObjRectangle rectBonus{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	_bonusSpawner->SpawnBonus(rectBonus, BonusType::Helmet);
	const auto shot{_allObjects.back()};

	shot->TakeDamage(1u, Author::Player1);

	EXPECT_FALSE(shot->GetIsAlive());
	EXPECT_EQ(_statistics->GetData().bonusDestroyedByPlayerOne, 1u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);

	_bonusSpawner->SpawnBonus(rectBonus, BonusType::Helmet);
	const auto taken{std::dynamic_pointer_cast<Bonus>(_allObjects.back())};
	ASSERT_NE(taken, nullptr);

	taken->PickUpBonus(Author::Player1);

	EXPECT_FALSE(taken->GetIsAlive());
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 1u);
	EXPECT_EQ(_statistics->GetData().bonusDestroyedByPlayerOne, 1u);
}
