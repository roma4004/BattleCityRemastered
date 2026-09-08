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
#include "enums/Faction.h"
#include <memory>

class CoopBotTest : public testing::Test// NOLINT(clang-diagnostic-padded)
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
	double _deltaTimeOneFrame{1.0 / 60.0};
	Uuid _uuid{};
	double _tankSize{};
	double _tankSpeed{142};
	double _gridSize{};
	unsigned short _tankHealth{100u};
	EventSubscription _spawnQueueSub{};

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

TEST_F(CoopBotTest, CoopNoChangeDirIfBonusOutsideLineOfSight)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn a bonus diagonally, out of the bot's direct line of sight
	_bonusSpawner->SpawnRandomBonus({.x = _tankSize * 2.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize});

	const Direction startDirCoop = coopBot->GetDirection();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const Direction endDirCoop = coopBot->GetDirection();

	EXPECT_EQ(startDirCoop, endDirCoop);
}

TEST_F(CoopBotTest, CoopShootToEnemy)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Enemy in line of sight
	const ObjRectangle rectEnemy{.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::UP, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 2u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const size_t sizeAfter = _allObjects.size();
	EXPECT_LT(sizeBefore, sizeAfter);// Bullet should be spawned
	EXPECT_EQ(sizeAfter, 4u);
}

TEST_F(CoopBotTest, CoopNoShootToCoop)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn another coopBot in line of sight first one
	const ObjRectangle coopBotRect2{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot2 =
			TestUtils::CreateBot(
					coopBotRect2, _tankHealth, _uuid, Author::Player2, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::UP, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot2);

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

TEST_F(CoopBotTest, CoopNoShootToPlayer1)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Player in line of sight Coop
	const ObjRectangle rectPlayer{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> player =
			TestUtils::CreatePlayer(
					rectPlayer, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::UP, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

TEST_F(CoopBotTest, CoopShootToBrick)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<BrickWall>(rect, _events, _uuid, _gameConfig));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(sizeBefore, _allObjects.size());
}

TEST_F(CoopBotTest, CoopTooCloseToShootTheBrick)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize + 18.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<BrickWall>(rect, _events, _uuid, _gameConfig));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

TEST_F(CoopBotTest, CoopShootToSteel)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 3u,
					_tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<SteelWall>(rect, _events, _uuid, _gameConfig));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(sizeBefore, _allObjects.size());
}

TEST_F(CoopBotTest, CoopNoShootToSteelIfTierTooLow)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<SteelWall>(rect, _events, _uuid, _gameConfig));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

TEST_F(CoopBotTest, CoopNoShootToEagle)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<EagleTile>(rect, _events, _uuid, _gameConfig));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

TEST_F(CoopBotTest, CoopNoShootToFortress)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<FortressBrickWall>(rect, _events, _uuid, _gameConfig));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

TEST_F(CoopBotTest, CoopNoShootToWater)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<WaterTile>(rect, _events, _uuid, _gameConfig));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

TEST_F(CoopBotTest, CoopNoShootToBush)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<BushTile>(rect, _events, _uuid, _gameConfig));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

TEST_F(CoopBotTest, CoopNoShootToIce)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	const ObjRectangle rect{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<IceTile>(rect, _events, _uuid, _gameConfig));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(sizeBefore, _allObjects.size());
}
