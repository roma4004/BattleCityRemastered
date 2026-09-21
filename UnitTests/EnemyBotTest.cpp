#include "TestUtils.h"
#include "components/ObstacleSpawner.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/TankPool.h"
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
#include "geometry/Point.h"
#include "gtest/gtest.h"
#include <memory>

class EnemyBotTest : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<TankPool> _tankPool{nullptr};
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
		_obstacleSpawner = std::make_unique<ObstacleSpawner>(_events, _gameConfig);
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_tankPool = std::make_shared<TankPool>(_events, _allObjects, _gameConfig, _bulletPool);
		_stateManager = std::make_shared<GameStateManager>(_events);
		TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, _gameConfig.gameMode, _respawnManager,
								 _tankSpawner);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, _allObjects, _gameConfig);
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
		_gridSize = _gameConfig.gridOffset;
		_tankSize = _gameConfig.tankSize;

		//NOTE: both rolls are pinned open, or every test that expects a shot at an obstacle would flake
		_gameConfig.botShootObstacleChance = 1.0;
		_gameConfig.botShootFortressChance = 1.0;

		_allObjects.reserve(4u);
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

	void SpawnObstacleArea(const ObjRectangle area, const ObstacleType type) const
	{
		TestUtils::SpawnObstacleArea(_events, _allObjects, area, type, _gameConfig);
	}

	std::shared_ptr<BaseObj> SpawnObstacle(const FPoint pos, const ObstacleType type) const
	{
		return TestUtils::SpawnObstacle(_events, _allObjects, pos, type, _gameConfig);
	}
};

TEST_F(EnemyBotTest, EnemyShootToCoop)
{
	CreateBot({.x = 0.0, .y = _tankSize * 3.0}, Author::Player1, Direction::UP);

	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter{_allObjects.size()};
	EXPECT_LT(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, sizeBefore + 2u);
}

TEST_F(EnemyBotTest, EnemyShootToPlayer1)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	CreatePlayer({.x = 0.0, .y = _tankSize * 3.0});

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter{_allObjects.size()};
	EXPECT_LT(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, sizeBefore + 1u);
}

TEST_F(EnemyBotTest, EnemyShootToPlayer2)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	CreatePlayer({.x = 0.0, .y = _tankSize * 3.0}, Author::Player2);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter{_allObjects.size()};
	EXPECT_LT(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, sizeBefore + 1u);
}

TEST_F(EnemyBotTest, EnemyNoShootToPlayer1IfTooClose)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	CreatePlayer({.x = 0.0, .y = _tankSize * 2.0 + 7.0});

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter{_allObjects.size()};
	EXPECT_LT(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, sizeBefore + 1u);
}

TEST_F(EnemyBotTest, EnemyNoShootToPlayer2IfTooClose)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	CreatePlayer({.x = 0.0, .y = _tankSize * 2.0 + 7.0}, Author::Player2);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter{_allObjects.size()};
	EXPECT_LT(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, sizeBefore + 1u);
}

TEST_F(EnemyBotTest, EnemyNoShootToAllied)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	CreateBot({.x = 0.0, .y = _tankSize * 3.0}, Author::Enemy2, Direction::DOWN);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

TEST_F(EnemyBotTest, EnemyNoShootToAlliedIfTooClose)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::UP);

	CreateBot({.x = 0.0, .y = _tankSize * 2.0 + 6.0}, Author::Enemy2, Direction::DOWN);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

TEST_F(EnemyBotTest, EnemyShootToBrick)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	SpawnObstacleArea(rect, ObstacleType::Brick);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(sizeBefore, _allObjects.size());
}

TEST_F(EnemyBotTest, EnemyTooCloseToShootTheBrick)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize + 3.0, .w = _tankSize, .h = _tankSize};
	SpawnObstacleArea(rect, ObstacleType::Brick);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

//NOTE: farther than the other cases - a tier-three blast on a wall one tank away would catch the shooter,
//and the bot holds its fire
TEST_F(EnemyBotTest, EnemyShootToSteel)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN, 3u);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0 + _gridSize, .w = _tankSize, .h = _tankSize};
	SpawnObstacleArea(rect, ObstacleType::Steel);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(sizeBefore, _allObjects.size());
}

TEST_F(EnemyBotTest, EnemyNoShootToSteelIfTierTooLow)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	SpawnObstacleArea(rect, ObstacleType::Steel);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

TEST_F(EnemyBotTest, EnemyShootToEagle)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	SpawnObstacle(FPoint{.x = 0.0, .y = _tankSize * 2.0}, ObstacleType::Eagle);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(sizeBefore, _allObjects.size());
}

// The two rolls are separate numbers - what a bot decides about a wall says nothing about
// what it decides about the base
TEST_F(EnemyBotTest, EnemyShootsTheEagleWithTheWallChanceAtZero)
{
	_gameConfig.botShootObstacleChance = 0.0;

	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	SpawnObstacle(FPoint{.x = 0.0, .y = _tankSize * 2.0}, ObstacleType::Eagle);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(sizeBefore, _allObjects.size());
}

TEST_F(EnemyBotTest, EnemyHoldsFireAtTheEagleWhenItsOwnChanceIsZero)
{
	_gameConfig.botShootFortressChance = 0.0;

	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	SpawnObstacle(FPoint{.x = 0.0, .y = _tankSize * 2.0}, ObstacleType::Eagle);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

TEST_F(EnemyBotTest, EnemyShootToFortress)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<FortressBrickWall>(rect, _events, _uuid, _gameConfig));

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(sizeBefore, _allObjects.size());
}

TEST_F(EnemyBotTest, EnemyShootToWater)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	SpawnObstacleArea(rect, ObstacleType::Water);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

TEST_F(EnemyBotTest, EnemyShootToBush)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	SpawnObstacleArea(rect, ObstacleType::Bush);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

TEST_F(EnemyBotTest, EnemyShootToIce)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	SpawnObstacleArea(rect, ObstacleType::Ice);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

TEST_F(EnemyBotTest, EnemyShootToPlayerBehindWater)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize};
	SpawnObstacleArea(rect, ObstacleType::Water);

	const auto player{CreatePlayer({.x = 0.0, .y = _tankSize * 3.0 + 2.0})};

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter{_allObjects.size()};
	EXPECT_LT(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, sizeBefore + 1u);
}

// A player standing in the water, which takes a Ship bonus, is still a target
TEST_F(EnemyBotTest, EnemyShootToPlayerInTheWater)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize};
	SpawnObstacleArea(rect, ObstacleType::Water);

	CreatePlayer({.x = 0.0, .y = _tankSize * 2.0 + 1.0});

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter{_allObjects.size()};
	EXPECT_LT(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, sizeBefore + 1u);
}

TEST_F(EnemyBotTest, EnemyShootToPlayerBehindIce)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize};
	SpawnObstacleArea(rect, ObstacleType::Ice);

	CreatePlayer({.x = 0.0, .y = _tankSize * 3.0 + 2.0});

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter{_allObjects.size()};
	EXPECT_LT(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, sizeBefore + 1u);
}

TEST_F(EnemyBotTest, EnemyShootToPlayerInTheIce)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize};
	SpawnObstacleArea(rect, ObstacleType::Ice);

	CreatePlayer({.x = 0.0, .y = _tankSize * 2.0 + 1.0});

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter{_allObjects.size()};
	EXPECT_LT(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, sizeBefore + 1u);
}

TEST_F(EnemyBotTest, EnemyNoShootToPlayerBehindBrickWall)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::RIGHT);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize};
	SpawnObstacleArea(rect, ObstacleType::Brick);

	CreatePlayer({.x = 0.0, .y = _tankSize * 3.0 + 2.0});

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter{_allObjects.size()};
	EXPECT_EQ(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, sizeBefore);
}

TEST_F(EnemyBotTest, EnemyNoShootToPlayerBehindSteelWall)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::RIGHT);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize};
	SpawnObstacleArea(rect, ObstacleType::Steel);

	CreatePlayer({.x = 0.0, .y = _tankSize * 3.0 + 2.0});

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter{_allObjects.size()};
	EXPECT_EQ(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, sizeBefore);
}

TEST_F(EnemyBotTest, EnemyNoShootToPlayerBehindFortressWall)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::RIGHT);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<FortressBrickWall>(rect, _events, _uuid, _gameConfig));

	CreatePlayer({.x = 0.0, .y = _tankSize * 3.0 + 2.0});

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter{_allObjects.size()};
	EXPECT_EQ(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, sizeBefore);
}

TEST_F(EnemyBotTest, EnemyNoShootToPlayerBehindBush)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize};
	SpawnObstacleArea(rect, ObstacleType::Bush);

	CreatePlayer({.x = 0.0, .y = _tankSize * 3.0 + 2.0});

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter{_allObjects.size()};
	EXPECT_EQ(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, sizeBefore);
}

TEST_F(EnemyBotTest, EnemyNoShootToPlayerInTheBush)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize};
	SpawnObstacleArea(rect, ObstacleType::Bush);

	CreatePlayer({.x = 0.0, .y = _tankSize * 2.0 + 1.0});

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter{_allObjects.size()};
	EXPECT_EQ(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, sizeBefore);
}
