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

// the same line-of-sight questions as the enemy bot, asked of one fighting on the players' side
class CoopBotTest : public testing::Test// NOLINT(clang-diagnostic-padded)
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
	double _deltaTimeOneFrame{1.0 / 60.0};
	Uuid _uuid{};
	double _tankSize{};
	double _gridSize{};
	unsigned short _tankHealth{100u};
	EventSubscription _spawnQueueSub{};

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

	std::shared_ptr<Tank> CreateBot(const FPoint pos, const Author author, const Direction dir,
									const unsigned short tier = 1u)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};
		auto bot{TestUtils::CreateBot(rect, _tankHealth, author, _allObjects, _events, dir, _tankPool, _gameConfig,
									  tier)};

		return bot;
	}

	std::shared_ptr<Tank> CreatePlayer(const FPoint pos, const Author author, const Direction dir)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};
		auto player{TestUtils::CreatePlayer(rect, _tankHealth, author, _allObjects, _events, dir, _tankPool,
											_gameConfig)};

		return player;
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

// a bonus off the line it drives is no reason to turn
TEST_F(CoopBotTest, CoopNoChangeDirIfBonusOutsideLineOfSight)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};

	_bonusSpawner->SpawnRandomBonus({.x = _tankSize * 2.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize});

	const Direction startDirCoop{coopBot->GetDirection()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const Direction endDirCoop{coopBot->GetDirection()};

	EXPECT_EQ(startDirCoop, endDirCoop);
}

// an enemy down the line is shot at
TEST_F(CoopBotTest, CoopShootToEnemy)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN);

	CreateBot({.x = 0.0, .y = _tankSize * 3.0}, Author::Enemy1, Direction::UP);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter{_allObjects.size()};
	EXPECT_LT(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, sizeBefore + 2u);
}

// another coop bot is not
TEST_F(CoopBotTest, CoopNoShootToCoop)
{
	const auto coopBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};

	CreateBot({.x = 0.0, .y = _tankSize * 3.0}, Author::Player2, Direction::UP);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

// and neither is the player it fights for
TEST_F(CoopBotTest, CoopNoShootToPlayer1)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN);

	CreatePlayer({.x = 0.0, .y = _tankSize * 3.0}, Author::Player1, Direction::UP);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

// a brick in the way is fired at once the roll comes up
TEST_F(CoopBotTest, CoopShootToBrick)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize};
	SpawnObstacleArea(rect, ObstacleType::Brick);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(sizeBefore, _allObjects.size());
}

// and left alone from a muzzle away
TEST_F(CoopBotTest, CoopTooCloseToShootTheBrick)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize + 18.0, .w = _tankSize, .h = _tankSize};
	SpawnObstacleArea(rect, ObstacleType::Brick);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

//NOTE: a cell farther than the other cases - a tier-three blast is wider, and the bot holds fire inside it
TEST_F(CoopBotTest, CoopShootToSteel)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN, 3u);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 3.0 + _gridSize, .w = _tankSize, .h = _tankSize};
	SpawnObstacleArea(rect, ObstacleType::Steel);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(sizeBefore, _allObjects.size());
}

// and not to one that does not
TEST_F(CoopBotTest, CoopNoShootToSteelIfTierTooLow)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize};
	SpawnObstacleArea(rect, ObstacleType::Steel);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

// the base is its own to defend, so it holds fire
TEST_F(CoopBotTest, CoopNoShootToEagle)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN);

	SpawnObstacle(FPoint{.x = 0.0, .y = _tankSize * 3.0}, ObstacleType::Eagle);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

// and leaves the wall around it standing
TEST_F(CoopBotTest, CoopNoShootToFortress)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<FortressBrickWall>(rect, _events, _uuid, _gameConfig));

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

// water is not shot at
TEST_F(CoopBotTest, CoopNoShootToWater)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize};
	SpawnObstacleArea(rect, ObstacleType::Water);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

// nor a bush
TEST_F(CoopBotTest, CoopNoShootToBush)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize};
	SpawnObstacleArea(rect, ObstacleType::Bush);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

// nor ice - none of them is in anyone's way
TEST_F(CoopBotTest, CoopNoShootToIce)
{
	CreateBot({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize};
	SpawnObstacleArea(rect, ObstacleType::Ice);

	const size_t sizeBefore{_allObjects.size()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}
