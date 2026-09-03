#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/TimingEvents.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "components/managers/GameStateManager.h"
#include "entities/obstacles/BushTile.h"
#include "entities/obstacles/IceTile.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/BulletCalibre.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include "enums/Faction.h"
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
	GameMode _gameMode{GameMode::OnePlayer};
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

		_allObjects.reserve(4u);
	}

	void TearDown() override {}
};

// Check that bots change direction to the opponent
TEST_F(BotsTest, BotsChangeDirectionIfOpponentSeen)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 3.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
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
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Player exactly where an enemy made the bot turn in the test above
	const ObjRectangle playerRect{.x = _tankSize * 3.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> player =
			TestUtils::CreatePlayer(
					playerRect, _tankHealth, _uuid, Author::Player2, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::DOWN);
}

// Check that two bots of the same team ignore each other
TEST_F(BotsTest, BotsNoChangeDirectionIfBotAllySeen)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn a second Coop in the same spot the enemy took above
	const ObjRectangle secondCoopRect{.x = _tankSize * 3.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> secondCoopBot =
			TestUtils::CreateBot(
					secondCoopRect, _tankHealth, _uuid, Author::Player2, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(secondCoopBot);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::DOWN);
	EXPECT_EQ(secondCoopBot->GetDirection(), Direction::DOWN);
}

// Check that a bot does not turn to an opponent it cannot shoot without catching its own blast
TEST_F(BotsTest, BotsNoChangeDirectionIfOpponentTooClose)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Enemy side by side, with no gap at all
	const ObjRectangle rectEnemy{.x = _tankSize, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::DOWN);
	EXPECT_EQ(enemyBot->GetDirection(), Direction::DOWN);
}

// Check that bots change their direction to the bonus
TEST_F(BotsTest, BotsChangeDirectionIfBonusSeenAndNoOneShoot)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	_bonusSpawner->SpawnRandomBonus({.x = _tankSize + 21.0, .y = 0.0, .w = _tankSize, .h = _tankSize});

	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 3.0 + 40.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
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

// Check that both Bots can't see bonus that behind water
TEST_F(BotsTest, BotsCantSeeBonusBehindWater)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::RIGHT, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Water
	_allObjects.emplace_back(
			std::make_shared<WaterTile>(
					ObjRectangle{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameMode));

	// Spawn Bonus
	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize * 3.0 + 2.0, .w = _tankSize, .h = _tankSize});

	// Spawn Water
	_allObjects.emplace_back(
			std::make_shared<WaterTile>(
					ObjRectangle{.x = 0.0, .y = _tankSize * 4.0 + 3.0, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameMode));

	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.0, .y = _tankSize * 5.0 + 40.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::RIGHT, _gameMode, _bulletPool, _gameConfig);
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

// Check that both Bots can't see bonus that behind Bush
TEST_F(BotsTest, BotsCantSeeBonusBehindBush)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::RIGHT, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Bush
	_allObjects.emplace_back(
			std::make_shared<BushTile>(
					ObjRectangle{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameMode));

	// Spawn Bonus
	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize * 3.0 + 2.0, .w = _tankSize, .h = _tankSize});

	// Spawn Bush
	_allObjects.emplace_back(
			std::make_shared<BushTile>(
					ObjRectangle{.x = 0.0, .y = _tankSize * 4.0 + 3.0, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameMode));

	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.0, .y = _tankSize * 5.0 + 40.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::RIGHT, _gameMode, _bulletPool, _gameConfig);
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

// Check that both Bots can see bonus that behind Ice
TEST_F(BotsTest, BotsCanSeeBonusBehindIce)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::RIGHT, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Ice
	_allObjects.emplace_back(
			std::make_shared<IceTile>(
					ObjRectangle{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameMode));

	// Spawn Bonus
	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize * 3.0 + 2.0, .w = _tankSize, .h = _tankSize});

	// Spawn Ice
	_allObjects.emplace_back(
			std::make_shared<IceTile>(
					ObjRectangle{.x = 0.0, .y = _tankSize * 4.0 + 3.0, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameMode));

	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.0, .y = _tankSize * 5.0 + 40.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::RIGHT, _gameMode, _bulletPool, _gameConfig);
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

// Check that both Bots can see bonus that in the Ice
TEST_F(BotsTest, BotsCanSeeBonusInTheIce)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> coopBot =
			TestUtils::CreateBot(
					coopBotRect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::RIGHT, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Ice
	_allObjects.emplace_back(
			std::make_shared<IceTile>(
					ObjRectangle{.x = 0.0, .y = _tankSize * 2.0 + 1.0, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameMode));

	// Spawn Ice
	_allObjects.emplace_back(
			std::make_shared<IceTile>(
					ObjRectangle{.x = 0.0, .y = _tankSize * 3.0 + 1.0, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameMode));

	// Spawn Bonus
	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize * 3.0 + 2.0, .w = _tankSize, .h = _tankSize});

	// Spawn Ice
	_allObjects.emplace_back(
			std::make_shared<IceTile>(
					ObjRectangle{.x = 0.0, .y = _tankSize * 4.0 + 3.0, .w = _tankSize, .h = _tankSize},
					_events,
					_uuid,
					_gameMode));

	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.0, .y = _tankSize * 5.0 + 40.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::RIGHT, _gameMode, _bulletPool, _gameConfig);
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
								 _events, 1u, _tankSpeed, Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
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
	_allObjects.emplace_back(TestUtils::CreateBullet(bulletRect, _tankHealth, _uuid, Faction::EnemyTeam, _allObjects, _events, calibre,
													 Direction::LEFT, _gameMode, _gameConfig, Author::Enemy1));

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
								 _events, 1u, _tankSpeed, Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
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
	_allObjects.emplace_back(TestUtils::CreateBullet(bulletRect, _tankHealth, _uuid, Faction::EnemyTeam, _allObjects, _events, calibre,
													 Direction::RIGHT, _gameMode, _gameConfig, Author::Enemy1));

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(coopBot->GetDirection(), Direction::RIGHT);
}
