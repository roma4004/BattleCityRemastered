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
#include "entities/obstacles/IceTile.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/BulletCalibre.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "gtest/gtest.h"
#include "enums/Faction.h"
#include <chrono>
#include <cstddef>
#include <memory>

class BotsTest : public testing::Test// NOLINT(clang-diagnostic-padded)
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

TEST_F(BotsTest, BotsChangeDirectionIfOpponentSeen)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	const ObjRectangle rectEnemy{.x = _tankSize * 3.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const Direction startDirCoop = coopBot->GetDirection();
	const Direction startDirEnemy = enemyBot->GetDirection();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const Direction endDirCoop = coopBot->GetDirection();
	const Direction endDirEnemy = enemyBot->GetDirection();

	EXPECT_NE(startDirCoop, endDirCoop);
	EXPECT_NE(startDirEnemy, endDirEnemy);
	EXPECT_EQ(endDirCoop, Direction::RIGHT);
	EXPECT_EQ(endDirEnemy, Direction::LEFT);
}

// Check that a bot does not aim at a seat on its own team - the mirror of the test above, same
// geometry, only the neighbour is an ally
TEST_F(BotsTest, BotsNoChangeDirectionIfPlayerAllySeen)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Player exactly where an enemy made the bot turn in the test above
	const ObjRectangle playerRect{.x = _tankSize * 3.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> player =
			TestUtils::CreatePlayer(
					playerRect, _tankHealth, _uuid, Author::Player2, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::DOWN);
}

TEST_F(BotsTest, BotsNoChangeDirectionIfBotAllySeen)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn a second Coop in the same spot the enemy took above
	const ObjRectangle secondCoopRect{.x = _tankSize * 3.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> secondCoopBot =
			TestUtils::CreateBot(
					secondCoopRect, _tankHealth, _uuid, Author::Player2, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(secondCoopBot);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::DOWN);
	EXPECT_EQ(secondCoopBot->GetDirection(), Direction::DOWN);
}

// A bot does not turn to an opponent it cannot shoot without catching its own blast
TEST_F(BotsTest, BotsNoChangeDirectionIfOpponentTooClose)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Enemy side by side, with no gap at all
	const ObjRectangle rectEnemy{.x = _tankSize, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::DOWN);
	EXPECT_EQ(enemyBot->GetDirection(), Direction::DOWN);
}

TEST_F(BotsTest, BotsChangeDirectionIfBonusSeenAndNoOneShoot)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	_bonusSpawner->SpawnRandomBonus({.x = _tankSize + 21.0, .y = 0.0, .w = _tankSize, .h = _tankSize});

	const ObjRectangle rectEnemy{.x = _tankSize * 3.0 + 40.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const Direction startDirCoop = coopBot->GetDirection();
	const Direction startDirEnemy = enemyBot->GetDirection();
	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const Direction endDirCoop = coopBot->GetDirection();
	const Direction endDirEnemy = enemyBot->GetDirection();

	EXPECT_EQ(sizeBefore, _allObjects.size());
	EXPECT_NE(startDirCoop, endDirCoop);
	EXPECT_NE(startDirEnemy, endDirEnemy);
	EXPECT_EQ(endDirCoop, Direction::RIGHT);
	EXPECT_EQ(endDirEnemy, Direction::LEFT);
}

TEST_F(BotsTest, BotsCantSeeBonusBehindWater)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::RIGHT, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	_allObjects.emplace_back(
			std::make_shared<WaterTile>(
					ObjRectangle{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameConfig));

	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize * 3.0 + 2.0, .w = _tankSize, .h = _tankSize});

	_allObjects.emplace_back(
			std::make_shared<WaterTile>(
					ObjRectangle{.x = 0.0, .y = _tankSize * 4.0 + 3.0, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameConfig));

	const ObjRectangle rectEnemy{.x = 0.0, .y = _tankSize * 5.0 + 40.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::RIGHT, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const Direction startDirCoop = coopBot->GetDirection();
	const Direction startDirEnemy = enemyBot->GetDirection();
	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const Direction endDirCoop = coopBot->GetDirection();
	const Direction endDirEnemy = enemyBot->GetDirection();

	EXPECT_EQ(sizeBefore, _allObjects.size());
	EXPECT_EQ(startDirCoop, endDirCoop);
	EXPECT_EQ(startDirEnemy, endDirEnemy);
	EXPECT_EQ(endDirCoop, Direction::RIGHT);
	EXPECT_EQ(endDirEnemy, Direction::RIGHT);
}

TEST_F(BotsTest, BotsCantSeeBonusBehindBush)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::RIGHT, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	_allObjects.emplace_back(
			std::make_shared<BushTile>(
					ObjRectangle{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameConfig));

	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize * 3.0 + 2.0, .w = _tankSize, .h = _tankSize});

	_allObjects.emplace_back(
			std::make_shared<BushTile>(
					ObjRectangle{.x = 0.0, .y = _tankSize * 4.0 + 3.0, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameConfig));

	const ObjRectangle rectEnemy{.x = 0.0, .y = _tankSize * 5.0 + 40.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::RIGHT, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const Direction startDirCoop = coopBot->GetDirection();
	const Direction startDirEnemy = enemyBot->GetDirection();
	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const Direction endDirCoop = coopBot->GetDirection();
	const Direction endDirEnemy = enemyBot->GetDirection();

	EXPECT_EQ(sizeBefore, _allObjects.size());
	EXPECT_EQ(startDirCoop, endDirCoop);
	EXPECT_EQ(startDirEnemy, endDirEnemy);
	EXPECT_EQ(endDirCoop, Direction::RIGHT);
	EXPECT_EQ(endDirEnemy, Direction::RIGHT);
}

TEST_F(BotsTest, BotsCanSeeBonusBehindIce)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::RIGHT, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	_allObjects.emplace_back(
			std::make_shared<IceTile>(
					ObjRectangle{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameConfig));

	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize * 3.0 + 2.0, .w = _tankSize, .h = _tankSize});

	_allObjects.emplace_back(
			std::make_shared<IceTile>(
					ObjRectangle{.x = 0.0, .y = _tankSize * 4.0 + 3.0, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameConfig));

	const ObjRectangle rectEnemy{.x = 0.0, .y = _tankSize * 5.0 + 40.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::RIGHT, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const Direction startDirCoop = coopBot->GetDirection();
	const Direction startDirEnemy = enemyBot->GetDirection();
	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const Direction endDirCoop = coopBot->GetDirection();
	const Direction endDirEnemy = enemyBot->GetDirection();

	EXPECT_EQ(sizeBefore, _allObjects.size());
	EXPECT_NE(startDirCoop, endDirCoop);
	EXPECT_NE(startDirEnemy, endDirEnemy);
	EXPECT_EQ(endDirCoop, Direction::DOWN);
	EXPECT_EQ(endDirEnemy, Direction::UP);
}

TEST_F(BotsTest, BotsCanSeeBonusInTheIce)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::RIGHT, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	_allObjects.emplace_back(
			std::make_shared<IceTile>(
					ObjRectangle{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameConfig));

	_allObjects.emplace_back(
			std::make_shared<IceTile>(
					ObjRectangle{.x = 0.0, .y = _tankSize * 3.0 + 1.0, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameConfig));

	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize * 3.0 + 2.0, .w = _tankSize, .h = _tankSize});

	_allObjects.emplace_back(
			std::make_shared<IceTile>(
					ObjRectangle{.x = 0.0, .y = _tankSize * 4.0 + 3.0, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameConfig));

	const ObjRectangle rectEnemy{.x = 0.0, .y = _tankSize * 5.0 + 40.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events, 1u,
					_tankSpeed, Direction::RIGHT, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const Direction startDirCoop = coopBot->GetDirection();
	const Direction startDirEnemy = enemyBot->GetDirection();
	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const Direction endDirCoop = coopBot->GetDirection();
	const Direction endDirEnemy = enemyBot->GetDirection();

	EXPECT_EQ(sizeBefore, _allObjects.size());
	EXPECT_NE(startDirCoop, endDirCoop);
	EXPECT_NE(startDirEnemy, endDirEnemy);
	EXPECT_EQ(endDirCoop, Direction::DOWN);
	EXPECT_EQ(endDirEnemy, Direction::UP);
}

// A bullet carries its shooter's faction, so a bot sees it as an opponent and fires
TEST_F(BotsTest, BotShootsAtAnIncomingBullet)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	const std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects,
								 _events, 1u, _tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	const double bulletWidth = coopBot->GetBulletWidth();
	const double bulletHeight = coopBot->GetBulletHeight();
	const BulletCalibre calibre{.speed = 300.0,
								.damage = 1u,
								.damageRadius = 12.0,
								.tier = 1u,
								.size{.x = bulletWidth, .y = bulletHeight}};

	// head-on: to the right of the bot and flying at it
	const ObjRectangle bulletRect{.x = _tankSize * 2.0,
								  .y = (_tankSize - bulletHeight) / 2.0,
								  .w = bulletWidth,
								  .h = bulletHeight};
	_allObjects.emplace_back(TestUtils::CreateBullet(bulletRect, _tankHealth, _uuid, Faction::EnemyTeam, _allObjects,
													 _events, calibre, Direction::LEFT, _gameConfig, Author::Enemy1));

	const std::size_t worldSizeBeforeShot = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::RIGHT);
	EXPECT_GT(_allObjects.size(), worldSizeBeforeShot);
}

// The same turn on a bullet flying away - nothing asks where it is headed
TEST_F(BotsTest, BotAimsAtABulletFlyingAwayJustTheSame)
{
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	const std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects,
								 _events, 1u, _tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	const double bulletWidth = coopBot->GetBulletWidth();
	const double bulletHeight = coopBot->GetBulletHeight();
	const BulletCalibre calibre{.speed = 300.0,
								.damage = 1u,
								.damageRadius = 12.0,
								.tier = 1u,
								.size{.x = bulletWidth, .y = bulletHeight}};

	const ObjRectangle bulletRect{.x = _tankSize * 2.0,
								  .y = (_tankSize - bulletHeight) / 2.0,
								  .w = bulletWidth,
								  .h = bulletHeight};
	_allObjects.emplace_back(TestUtils::CreateBullet(bulletRect, _tankHealth, _uuid, Faction::EnemyTeam, _allObjects,
													 _events, calibre, Direction::RIGHT, _gameConfig, Author::Enemy1));

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::RIGHT);
}

// The cooldown gate lives in Tank::TickUpdate, not in ShouldShoot - one shot per cooldown, no matter
// how long the target stays in sight
TEST_F(BotsTest, BotDoesNotShootTwiceWithinOneCooldown)
{
	const ObjRectangle botRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	const std::shared_ptr<Tank> bot =
			TestUtils::CreateBot(botRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects,
								 _events, 1u, _tankSpeed, Direction::RIGHT, _bulletPool, _gameConfig);
	_allObjects.emplace_back(bot);

	const ObjRectangle enemyRect{.x = _tankSize * 3.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(TestUtils::CreateBot(enemyRect, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam,
												  _allObjects, _events, 1u, _tankSpeed, Direction::LEFT, _bulletPool,
												  _gameConfig));

	const std::size_t beforeFirstShot = _allObjects.size();
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	const std::size_t afterFirstShot = _allObjects.size();
	ASSERT_GT(afterFirstShot, beforeFirstShot);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_allObjects.size(), afterFirstShot);
}

// The cooldown suppresses aiming, not only firing: ChangeDirIfSeenOpponent bails on !CanShoot(), so a
// reloading bot ignores a target that appears on another side
TEST_F(BotsTest, ReloadingBotDoesNotTurnToANewOpponent)
{
	const ObjRectangle botRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	const std::shared_ptr<Tank> bot =
			TestUtils::CreateBot(botRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects,
								 _events, 1u, _tankSpeed, Direction::RIGHT, _bulletPool, _gameConfig);
	_allObjects.emplace_back(bot);

	const ObjRectangle rightEnemyRect{.x = _tankSize * 3.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(TestUtils::CreateBot(rightEnemyRect, _tankHealth, _uuid, Author::Enemy1,
												  Faction::EnemyTeam, _allObjects, _events, 1u, _tankSpeed,
												  Direction::LEFT, _bulletPool, _gameConfig));

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	ASSERT_EQ(bot->GetDirection(), Direction::RIGHT);

	// a second target below, while the first shot is still cooling down
	const ObjRectangle belowEnemyRect{.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(TestUtils::CreateBot(belowEnemyRect, _tankHealth, _uuid, Author::Enemy2,
												  Faction::EnemyTeam, _allObjects, _events, 1u, _tankSpeed,
												  Direction::UP, _bulletPool, _gameConfig));

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(bot->GetDirection(), Direction::RIGHT);
}

// Nose to the wall: the move fails and steel cannot be shot, so the only way out is to turn. The gap
// is the padding movement parks on - flush against the wall every side reads as blocked
TEST_F(BotsTest, BotTurnsWhenItRunsIntoAWall)
{
	const ObjRectangle botRect{.x = _tankSize * 2.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	const std::shared_ptr<Tank> bot =
			TestUtils::CreateBot(botRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects,
								 _events, 1u, _tankSpeed, Direction::RIGHT, _bulletPool, _gameConfig);
	_allObjects.emplace_back(bot);

	// flush against the bot's right side, so the very first step is blocked
	_allObjects.emplace_back(std::make_shared<SteelWall>(
			ObjRectangle{.x = _tankSize * 3.0 + 1.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize},
			_events, _uuid, _gameConfig));

	ASSERT_EQ(bot->GetDirection(), Direction::RIGHT);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_NE(bot->GetDirection(), Direction::RIGHT);
}

// A bot must not fire into something closer than its own blast radius. Walled in on all four sides,
// so no random turn can move the target out of the way.
TEST_F(BotsTest, BotHoldsFireWhenTheBlastWouldReachItself)
{
	const ObjRectangle botRect{.x = _tankSize, .y = _tankSize, .w = _tankSize, .h = _tankSize};
	const std::shared_ptr<Tank> bot =
			TestUtils::CreateBot(botRect, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events,
								 1u, _tankSpeed, Direction::RIGHT, _bulletPool, _gameConfig);
	_allObjects.emplace_back(bot);

	for (const ObjRectangle& wallRect: {ObjRectangle{.x = _tankSize * 2.0, .y = _tankSize,
													   .w = _tankSize, .h = _tankSize},
										   ObjRectangle{.x = 0.0, .y = _tankSize, .w = _tankSize, .h = _tankSize},
										   ObjRectangle{.x = _tankSize, .y = 0.0, .w = _tankSize, .h = _tankSize},
										   ObjRectangle{.x = _tankSize, .y = _tankSize * 2.0,
														.w = _tankSize, .h = _tankSize}})
	{
		_allObjects.emplace_back(std::make_shared<BrickWall>(wallRect, _events, _uuid, _gameConfig));
	}

	const std::size_t before = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(bot->GetDirection(), Direction::RIGHT);
	EXPECT_EQ(_allObjects.size(), before);
}

// A bot has no keyboard to ignore, so Deactivate has to drop the tick subscription itself
TEST_F(BotsTest, ADeactivatedBotDoesNotDrive)
{
	const ObjRectangle botRect{.x = _tankSize * 3.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize};
	const std::shared_ptr<Tank> bot =
			TestUtils::CreateBot(botRect, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events,
								 1u, _tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(bot);

	const FPoint start = bot->GetPos();
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	ASSERT_NE(bot->GetPos(), start) << "the control failed - this bot does not drive even when active";

	bot->Deactivate();
	const FPoint parked = bot->GetPos();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(bot->GetPos(), parked);
}

// The chance gates walls and nothing else - a bot with a target in its sights fires whatever the
// wall roll would have said
TEST_F(BotsTest, BotShootsAnOpponentEvenWithTheWallChanceAtZero)
{
	_gameConfig.botShootObstacleChance = 0.0;

	const ObjRectangle botRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	const std::shared_ptr<Tank> bot =
			TestUtils::CreateBot(botRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects,
								 _events, 1u, _tankSpeed, Direction::RIGHT, _bulletPool, _gameConfig);
	_allObjects.emplace_back(bot);

	const ObjRectangle enemyRect{.x = _tankSize * 3.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(TestUtils::CreateBot(enemyRect, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam,
												  _allObjects, _events, 1u, _tankSpeed, Direction::LEFT, _bulletPool,
												  _gameConfig));

	const std::size_t before = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(_allObjects.size(), before);
}

// A wall in plain sight and in range, and still no shot - the roll is what decides, and at zero it
// decides once per sighting, so no number of frames wears it down
TEST_F(BotsTest, BotHoldsFireAtAWallWhenTheChanceIsZero)
{
	_gameConfig.botShootObstacleChance = 0.0;

	const ObjRectangle botRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	const std::shared_ptr<Tank> bot =
			TestUtils::CreateBot(botRect, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events,
								 1u, _tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(bot);

	_allObjects.emplace_back(std::make_shared<BrickWall>(
			ObjRectangle{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize}, _events, _uuid, _gameConfig));

	const std::size_t before = _allObjects.size();

	for (int frame{0}; frame < 10; ++frame)
	{
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	}

	EXPECT_EQ(_allObjects.size(), before);
	ASSERT_EQ(bot->GetDirection(), Direction::DOWN) << "the control failed - the bot looked away from the wall";
}

// The other end of the same dial: at one the wall is shot the first frame it is seen
TEST_F(BotsTest, BotShootsAWallWhenTheChanceIsOne)
{
	_gameConfig.botShootObstacleChance = 1.0;

	const ObjRectangle botRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	const std::shared_ptr<Tank> bot =
			TestUtils::CreateBot(botRect, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events,
								 1u, _tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(bot);

	_allObjects.emplace_back(std::make_shared<BrickWall>(
			ObjRectangle{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize}, _events, _uuid, _gameConfig));

	const std::size_t before = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(_allObjects.size(), before);
}

// A bot counts as blocked only flush against the wall, and that close its own blast would reach it -
// the way out of a dead end is the turn, never the gun.
TEST_F(BotsTest, ABlockedBotStillTurnsAwayAtZeroChance)
{
	_gameConfig.botShootObstacleChance = 0.0;

	const ObjRectangle botRect{.x = _tankSize * 2.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	const std::shared_ptr<Tank> bot =
			TestUtils::CreateBot(botRect, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events,
								 1u, _tankSpeed, Direction::RIGHT, _bulletPool, _gameConfig);
	_allObjects.emplace_back(bot);

	// flush against the bot's right side, destructible, and still not worth a shot from this close
	_allObjects.emplace_back(std::make_shared<BrickWall>(
			ObjRectangle{.x = _tankSize * 3.0 + 1.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize},
			_events, _uuid, _gameConfig));

	const std::size_t before = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_NE(bot->GetDirection(), Direction::RIGHT);
	EXPECT_EQ(_allObjects.size(), before);
}

// A refusal has to expire, or one unlucky roll decides for as long as that wall is in the sights.
// Cooldown set to nothing, so the next frame asks again with the chance flipped to the other end.
TEST_F(BotsTest, ARefusedWallIsReconsideredOnceTheCooldownIsUp)
{
	_gameConfig.botShootObstacleChance = 0.0;
	_gameConfig.botObstacleShootCooldown = std::chrono::milliseconds{0};

	const ObjRectangle botRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	const std::shared_ptr<Tank> bot =
			TestUtils::CreateBot(botRect, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events,
								 1u, _tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(bot);

	_allObjects.emplace_back(std::make_shared<BrickWall>(
			ObjRectangle{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize}, _events, _uuid, _gameConfig));

	const std::size_t before = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	ASSERT_EQ(_allObjects.size(), before) << "the control failed - a zero chance fired";

	_gameConfig.botShootObstacleChance = 1.0;
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(_allObjects.size(), before) << "the refusal outlived its cooldown";
}

// And the other half: until that cooldown is up the answer stands, or the roll is back to per frame
TEST_F(BotsTest, AWallRefusedStaysRefusedUntilTheCooldownIsUp)
{
	_gameConfig.botShootObstacleChance = 0.0;
	_gameConfig.botObstacleShootCooldown = std::chrono::minutes{1};

	const ObjRectangle botRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	const std::shared_ptr<Tank> bot =
			TestUtils::CreateBot(botRect, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events,
								 1u, _tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
	_allObjects.emplace_back(bot);

	_allObjects.emplace_back(std::make_shared<BrickWall>(
			ObjRectangle{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize}, _events, _uuid, _gameConfig));

	const std::size_t before = _allObjects.size();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	_gameConfig.botShootObstacleChance = 1.0;

	for (int frame{0}; frame < 10; ++frame)
	{
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	}

	EXPECT_EQ(_allObjects.size(), before) << "the answer was re-rolled before its cooldown was up";
}
