#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/TankPool.h"
#include "components/EventSystem.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/TimingEvents.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "components/managers/BonusManager.h"
#include "components/ObstacleSpawner.h"
#include "components/managers/FortressManager.h"
#include "entities/obstacles/FortressWalls.h"
#include "entities/pawns/Tank.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/ObstacleType.h"
#include "enums/InputChannel.h"
#include "gtest/gtest.h"
#include "enums/Faction.h"
#include <memory>

// the shovel in an enemy's hands is the mirror of the player's: instead of upgrading the fortress wall
// it takes the wall away, whatever the wall is made of
class BonusTestEnemy : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<TankPool> _tankPool{nullptr};
	std::unique_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::vector<EventSubscription> _instantSpawnAnimationSubs{};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<RespawnManager> _respawnManager{nullptr};
	std::shared_ptr<BonusManager> _bonusManager{nullptr};
	std::unique_ptr<FortressManager> _fortressManager{nullptr};
	std::unique_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::shared_ptr<BaseObj> _fortressWall{nullptr};
	EventSubscription _fortressWallSub{};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.0 / 60.0};
	double _tankSize{};
	double _gridSize{};
	unsigned short _tankHealth{100u};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_tankPool = std::make_shared<TankPool>(_events, _allObjects, _gameConfig, _bulletPool);
		TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, _gameConfig.gameMode, _respawnManager,
								 _tankSpawner);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, _allObjects, _gameConfig);
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
		_bonusManager = std::make_unique<BonusManager>(_events, _gameConfig);
		_fortressManager = std::make_unique<FortressManager>(_events, _allObjects, _gameConfig);
		_obstacleSpawner = std::make_unique<ObstacleSpawner>(_events, _gameConfig);
		_fortressWallSub = TestUtils::TrackFortressWall(_events, &_fortressWall);
		_gridSize = _gameConfig.gridOffset;
		_tankSize = _gameConfig.tankSize;
	}

	void TearDown() override {}

	std::shared_ptr<Tank> CreateBot(const FPoint pos, const Author author, const Direction dir)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};
		auto bot{TestUtils::CreateBot(rect, _tankHealth, author, _allObjects, _events, dir, _tankPool, _gameConfig)};

		return bot;
	}

	std::shared_ptr<Tank> CreatePlayer(const FPoint pos, const Author author, const Direction dir)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};
		auto player{TestUtils::CreatePlayer(rect, _tankHealth, author, _allObjects, _events, dir, _tankPool,
											_gameConfig)};

		return player;
	}
};


// a star grows an enemy as it grows a player - the volley of the higher tiers comes with it
TEST_F(BonusTestEnemy, StarPickUpByEnemyRaisesItsTier)
{
	const auto enemyBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN)};
	const unsigned int tier{enemyBot->GetTier()};

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Star);
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(enemyBot->GetTier(), tier + 1u);
}

// and so does the caliber, three tiers at once
TEST_F(BonusTestEnemy, CaliberPickUpByEnemyRaisesItsTierByThree)
{
	const auto enemyBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN)};
	const unsigned int tier{enemyBot->GetTier()};

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Caliber);
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(enemyBot->GetTier(), tier + 3u);
}

// the wall is brick when the enemy takes the shovel
TEST_F(BonusTestEnemy, ShovelPickUpByEnemyThenFortressBricWallkHide)
{
	const auto enemyBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN)};

	const ObjRectangle fortressRect{.x = _tankSize + 1.0, .y = 0, .w = _gridSize, .h = _gridSize};
	_events->EmitEvent(SpawnObstacleEvent{.rect = fortressRect, .type = ObstacleType::Fortress});

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Shovel);

	EXPECT_NE(dynamic_cast<FortressBrickWall*>(_fortressWall.get()), nullptr);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(_fortressWall->GetIsAlive());
}

// a player takes a shovel first, so the enemy's finds steel - it goes just the same
TEST_F(BonusTestEnemy, ShovelPickUpByEnemyThenFortressSteelWallHide)
{
	const auto enemyBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN)};

	_allObjects.reserve(4);
	CreatePlayer({.x = _tankSize * 2.0, .y = _tankSize * 2.0}, Author::Player1, Direction::DOWN);
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});

	const ObjRectangle fortressRect{.x = _tankSize * 3.0 + 1.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize};
	_events->EmitEvent(SpawnObstacleEvent{.rect = fortressRect, .type = ObstacleType::Fortress});

	const ObjRectangle enemyBonusRect{.x = 0.0, .y = _tankSize + 3.0, .w = _tankSize, .h = _tankSize};

	_bonusSpawner->SpawnBonus(enemyBonusRect, BonusType::Shovel);
	const ObjRectangle playerBonusRect{.x = _tankSize * 2.0,
									   .y = _tankSize * 2.0 + _tankSize + 1.0,
									   .w = _tankSize,
									   .h = _tankSize};
	_bonusSpawner->SpawnBonus(playerBonusRect, BonusType::Shovel);

	EXPECT_NE(dynamic_cast<FortressBrickWall*>(_fortressWall.get()), nullptr);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_NE(dynamic_cast<FortressSteelWall*>(_fortressWall.get()), nullptr);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(_fortressWall->GetIsAlive());
}

// NOTE: the enemy shovel ends the player's effect - left running, it swallows the next pickup as an extension
// the enemy's shovel takes the walls away and leaves the eagle's health alone
TEST_F(BonusTestEnemy, ShovelPickUpByEnemyLeavesTheEagleWounded)
{
	_events->EmitEvent(SpawnObstacleEvent{.rect = {.x = 0.0, .y = 0.0, .w = _gridSize, .h = _gridSize},
										  .type = ObstacleType::Eagle});
	const std::shared_ptr<BaseObj> eagle{_allObjects.back()};
	eagle->TakeDamage(1u, Author::Player1);
	const int woundedHealth{eagle->GetHealth()};

	_events->EmitEvent(BonusShovelPickupEvent{.faction = Faction::EnemyTeam});

	EXPECT_EQ(woundedHealth, eagle->GetHealth());
}

TEST_F(BonusTestEnemy, PlayerShovelWorksAgainAfterAnEnemyShovel)
{
	const ObjRectangle fortressRect{.x = _tankSize * 3.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize};
	_events->EmitEvent(SpawnObstacleEvent{.rect = fortressRect, .type = ObstacleType::Fortress});
	ASSERT_NE(dynamic_cast<FortressBrickWall*>(_fortressWall.get()), nullptr);

	_events->EmitEvent(BonusShovelPickupEvent{.faction = Faction::PlayerTeam});
	ASSERT_NE(dynamic_cast<FortressSteelWall*>(_fortressWall.get()), nullptr);

	_events->EmitEvent(BonusShovelPickupEvent{.faction = Faction::EnemyTeam});
	ASSERT_FALSE(_fortressWall->GetIsAlive());

	_events->EmitEvent(BonusShovelPickupEvent{.faction = Faction::PlayerTeam});

	EXPECT_NE(dynamic_cast<FortressSteelWall*>(_fortressWall.get()), nullptr);
	EXPECT_TRUE(_fortressWall->GetIsAlive());
}
