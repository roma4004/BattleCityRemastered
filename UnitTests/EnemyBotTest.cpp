#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/TimingEvents.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "components/managers/GameStateManager.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/BushTile.h"
#include "entities/obstacles/EagleTile.h"
#include "entities/obstacles/FortressWalls.h"
#include "entities/obstacles/IceTile.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "gtest/gtest.h"
#include <memory>

class EnemyBotTest : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::unique_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<GameStateManager> _stateManager{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<RespawnManager> _respawnManager{nullptr};
	std::vector<EventSubscription> _instantSpawnAnimationSubs{};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	EventSubscription _spawnQueueSub{};
	double _deltaTimeOneFrame{1.0 / 60.0};
	Uuid _uuid{};
	double _tankSize{};
	double _gridSize{};
	unsigned short _tankHealth{100u};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_stateManager = std::make_shared<GameStateManager>(_events);
		TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, _gameConfig.gameMode, _respawnManager,
								 _tankSpawner);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, _allObjects, _gameConfig);
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
		_gridSize = _gameConfig.gridOffset;
		_tankSize = _gridSize * 3.0;// for better turns

		//NOTE: the wall roll is pinned open, or every test that expects a shot at an obstacle would flake
		_gameConfig.botShootObstacleChance = 1.0;

		_allObjects.reserve(4u);
	}

	void TearDown() override {}
};

TEST_F(EnemyBotTest, EnemyShootToCoop)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, _allObjects, _events, 1u, Direction::UP, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Enemy in line of sight
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 2u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter = _allObjects.size();
	EXPECT_LT(sizeBefore, sizeAfter);// Bullet should be spawned
	EXPECT_EQ(sizeAfter, 4u);
}

TEST_F(EnemyBotTest, EnemyShootToPlayer1)
{
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::DOWN, _bulletPool, _gameConfig);

	_allObjects.emplace_back(enemyBot);

	// Spawn Player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> player =
			TestUtils::CreatePlayer(
					rectPlayer, _tankHealth, _uuid, Author::Player1, _allObjects, _events, 1u, Direction::UP, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 2u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter = _allObjects.size();
	EXPECT_LT(sizeBefore, sizeAfter);// Bullet should be spawned
	EXPECT_EQ(sizeAfter, 3u);
}

TEST_F(EnemyBotTest, EnemyShootToPlayer2)
{
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn Player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> player =
			TestUtils::CreatePlayer(
					rectPlayer, _tankHealth, _uuid, Author::Player2, _allObjects, _events, 1u, Direction::UP, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 2u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter = _allObjects.size();
	EXPECT_LT(sizeBefore, sizeAfter);// Bullet should be spawned
	EXPECT_EQ(sizeAfter, 3u);
}

TEST_F(EnemyBotTest, EnemyNoShootToPlayer1IfTooClose)
{
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn Player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.0, .y = _tankSize * 2.0 + 7.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> player =
			TestUtils::CreatePlayer(
					rectPlayer, _tankHealth, _uuid, Author::Player1, _allObjects, _events, 1u, Direction::UP, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 2u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter = _allObjects.size();
	EXPECT_LT(sizeBefore, sizeAfter);// Bullet should be spawned
	EXPECT_EQ(sizeAfter, 3u);
}

TEST_F(EnemyBotTest, EnemyNoShootToPlayer2IfTooClose)
{
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn Player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.0, .y = _tankSize * 2.0 + 7.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> player =
			TestUtils::CreatePlayer(
					rectPlayer, _tankHealth, _uuid, Author::Player2, _allObjects, _events, 1u, Direction::UP, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 2u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter = _allObjects.size();
	EXPECT_LT(sizeBefore, sizeAfter);// Bullet should be spawned
	EXPECT_EQ(sizeAfter, 3u);
}

// check that enemy don't shoot the allied tanks
TEST_F(EnemyBotTest, EnemyNoShootToAllied)
{
	// Spawn first Enemy
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn second Enemy in line of sight of the first
	const ObjRectangle rectEnemy2{.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<BaseObj> enemyBot2BaseObj =
			TestUtils::CreateBot(
					rectEnemy2, _tankHealth, _uuid, Author::Enemy2, _allObjects, _events, 1u, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot2BaseObj);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 2u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

// check that enemy doesn't shoot the allied tanks even if too close to them
TEST_F(EnemyBotTest, EnemyNoShootToAlliedIfTooClose)
{
	// Spawn first enemy
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::UP, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn second Enemy in line of sight of the first
	const ObjRectangle rectEnemy2{.x = 0.0, .y = _tankSize * 2.0 + 6.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<BaseObj> enemyBot2BaseObj =
			TestUtils::CreateBot(
					rectEnemy2, _tankHealth, _uuid, Author::Enemy2, _allObjects, _events, 1u, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot2BaseObj);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 2u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

TEST_F(EnemyBotTest, EnemyShootToBrick)
{
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<BrickWall>(rect, _events, _uuid, _gameConfig));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(sizeBefore, _allObjects.size());
}

TEST_F(EnemyBotTest, EnemyTooCloseToShootTheBrick)
{
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize + 3.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<BrickWall>(rect, _events, _uuid, _gameConfig));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

TEST_F(EnemyBotTest, EnemyShootToSteel)
{
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 3u, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<SteelWall>(rect, _events, _uuid, _gameConfig));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(sizeBefore, _allObjects.size());
}

TEST_F(EnemyBotTest, EnemyNoShootToSteelIfTierTooLow)
{
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<SteelWall>(rect, _events, _uuid, _gameConfig));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

TEST_F(EnemyBotTest, EnemyShootToEagle)
{
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<EagleTile>(rect, _events, _uuid, _gameConfig));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(sizeBefore, _allObjects.size());
}

TEST_F(EnemyBotTest, EnemyShootToFortress)
{
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<FortressBrickWall>(rect, _events, _uuid, _gameConfig));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(sizeBefore, _allObjects.size());
}

TEST_F(EnemyBotTest, EnemyShootToWater)
{
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<WaterTile>(rect, _events, _uuid, _gameConfig));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

TEST_F(EnemyBotTest, EnemyShootToBush)
{
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<BushTile>(rect, _events, _uuid, _gameConfig));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

TEST_F(EnemyBotTest, EnemyShootToIce)
{
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<IceTile>(rect, _events, _uuid, _gameConfig));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

TEST_F(EnemyBotTest, EnemyShootToPlayerBehindWater)
{
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<WaterTile>(rect, _events, _uuid, _gameConfig));

	// Spawn player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.0, .y = _tankSize * 3.0 + 2.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> player =
			TestUtils::CreatePlayer(
					rectPlayer, _tankHealth, _uuid, Author::Player1, _allObjects, _events, 1u, Direction::UP, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 3u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter = _allObjects.size();
	EXPECT_LT(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, 4u);
}

// A player standing in the water, which takes a Ship bonus, is still a target
TEST_F(EnemyBotTest, EnemyShootToPlayerInTheWater)
{
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<WaterTile>(rect, _events, _uuid, _gameConfig));

	// Spawn player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> player =
			TestUtils::CreatePlayer(
					rectPlayer, _tankHealth, _uuid, Author::Player1, _allObjects, _events, 1u, Direction::UP, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 3u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter = _allObjects.size();
	EXPECT_LT(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, 4u);
}

TEST_F(EnemyBotTest, EnemyShootToPlayerBehindIce)
{
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<IceTile>(rect, _events, _uuid, _gameConfig));

	// Spawn player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.0, .y = _tankSize * 3.0 + 2.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> player =
			TestUtils::CreatePlayer(
					rectPlayer, _tankHealth, _uuid, Author::Player1, _allObjects, _events, 1u, Direction::UP, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 3u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter = _allObjects.size();
	EXPECT_LT(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, 4u);
}

TEST_F(EnemyBotTest, EnemyShootToPlayerInTheIce)
{
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<IceTile>(rect, _events, _uuid, _gameConfig));

	// Spawn player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> player =
			TestUtils::CreatePlayer(
					rectPlayer, _tankHealth, _uuid, Author::Player1, _allObjects, _events, 1u, Direction::UP, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 3u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter = _allObjects.size();
	EXPECT_LT(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, 4u);
}

TEST_F(EnemyBotTest, EnemyNoShootToPlayerBehindBrickWall)
{
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::RIGHT, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<BrickWall>(rect, _events, _uuid, _gameConfig));

	// Spawn player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.0, .y = _tankSize * 3.0 + 2.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> player =
			TestUtils::CreatePlayer(
					rectPlayer, _tankHealth, _uuid, Author::Player1, _allObjects, _events, 1u, Direction::UP, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 3u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter = _allObjects.size();
	EXPECT_EQ(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, 3u);
}

TEST_F(EnemyBotTest, EnemyNoShootToPlayerBehindSteelWall)
{
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::RIGHT, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<SteelWall>(rect, _events, _uuid, _gameConfig));

	// Spawn player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.0, .y = _tankSize * 3.0 + 2.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> player =
			TestUtils::CreatePlayer(
					rectPlayer, _tankHealth, _uuid, Author::Player1, _allObjects, _events, 1u, Direction::UP, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 3u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter = _allObjects.size();
	EXPECT_EQ(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, 3u);
}

TEST_F(EnemyBotTest, EnemyNoShootToPlayerBehindFortressWall)
{
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::RIGHT, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<FortressBrickWall>(rect, _events, _uuid, _gameConfig));

	// Spawn player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.0, .y = _tankSize * 3.0 + 2.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> player =
			TestUtils::CreatePlayer(
					rectPlayer, _tankHealth, _uuid, Author::Player1, _allObjects, _events, 1u, Direction::UP, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 3u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter = _allObjects.size();
	EXPECT_EQ(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, 3u);
}

TEST_F(EnemyBotTest, EnemyNoShootToPlayerBehindBush)
{
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<BushTile>(rect, _events, _uuid, _gameConfig));

	// Spawn player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.0, .y = _tankSize * 3.0 + 2.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> player =
			TestUtils::CreatePlayer(
					rectPlayer, _tankHealth, _uuid, Author::Player1, _allObjects, _events, 1u, Direction::UP, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 3u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter = _allObjects.size();
	EXPECT_EQ(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, 3u);
}

TEST_F(EnemyBotTest, EnemyNoShootToPlayerInTheBush)
{
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<BushTile>(rect, _events, _uuid, _gameConfig));

	// Spawn player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> player =
			TestUtils::CreatePlayer(
					rectPlayer, _tankHealth, _uuid, Author::Player1, _allObjects, _events, 1u, Direction::UP, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 3u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter = _allObjects.size();
	EXPECT_EQ(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, 3u);
}
