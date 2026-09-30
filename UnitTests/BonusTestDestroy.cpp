#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/TankPool.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/TimingEvents.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "components/managers/BonusManager.h"
#include "entities/bonuses/Bonus.h"
#include "components/ObstacleSpawner.h"
#include "components/managers/FortressManager.h"
#include "entities/obstacles/FortressWalls.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/BulletResetProperty.h"
#include "entities/pawns/Tank.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/ObstacleType.h"
#include "geometry/Point.h"
#include "gtest/gtest.h"
#include <memory>

// the other half of the bonus pair: a bonus shot instead of picked up gives its effect to nobody
class BonusTestDestroy : public testing::Test// NOLINT(clang-diagnostic-padded)
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
	BulletCaliber _caliber{.speed = 300.0, .damage = 1u, .damageRadius = 12.0, .tier = 1u, .size{.x = 6.0, .y = 5.0}};
	double _tankSize{};
	double _gridSize{};
	unsigned short _tankHealth{100u};
	unsigned short _bulletHealth{1u};
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

		_allObjects.reserve(4);
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
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _caliber.size.x, .h = _caliber.size.y};
		auto bullet{TestUtils::CreateBullet(rect, _bulletHealth, _bulletPool, _events, _caliber, dir,
											author)};

		return bullet;
	}
};

// a shot bonus leaves the field
TEST_F(BonusTestDestroy, BonusDestroy)
{
	CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Enemy1);

	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = 7.0, .w = _tankSize, .h = _tankSize});

	if (const auto bonus{dynamic_cast<Bonus*>(_allObjects.back().get())})
	{
		EXPECT_TRUE(bonus->GetIsAlive());

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_FALSE(bonus->GetIsAlive());

		return;
	}

	EXPECT_TRUE(false);
}

// a shot that misses leaves it lying there
TEST_F(BonusTestDestroy, BonusNotDestroy)
{
	CreateBullet({.x = 0.0, .y = 0.0}, Direction::RIGHT, Author::Enemy1);

	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = 7.0, .w = _tankSize, .h = _tankSize});

	if (const auto bonus{dynamic_cast<Bonus*>(_allObjects.back().get())})
	{
		EXPECT_TRUE(bonus->GetIsAlive());

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_TRUE(bonus->GetIsAlive());

		return;
	}

	EXPECT_TRUE(false);
}

// a shot timer freezes nobody
TEST_F(BonusTestDestroy, TimerDestroyByPlayerAndEnemyStillMove)
{
	CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = 7.0, .w = _tankSize, .h = _tankSize}, BonusType::Timer);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const auto enemyBot{CreateBot({.x = _tankSize * 2, .y = _tankSize * 2}, Author::Enemy1, Direction::DOWN)};

	const FPoint enemyPos{enemyBot->GetPos()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_NE(enemyPos, enemyBot->GetPos());
}

// a shot helmet shields nobody
TEST_F(BonusTestDestroy, HelmetDestroyAndBulletStillCanDamageTank)
{
	const auto player{CreatePlayer({.x = _tankSize + 1.0, .y = 0.0}, Author::Player2)};
	CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = 7.0, .w = _tankSize, .h = _tankSize}, BonusType::Helmet);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	CreateBullet({.x = _tankSize * 2 + 1.0, .y = 7.0}, Direction::LEFT, Author::Enemy1);

	const int playerHealth{player->GetHealth()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_NE(playerHealth, player->GetHealth());
}


// a shot grenade leaves the enemy team at full health
TEST_F(BonusTestDestroy, GrenadeDestroyEnemyHealthFull)
{
	CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = 7.0, .w = _tankSize, .h = _tankSize}, BonusType::Grenade);

	const auto enemyBot{CreateBot({.x = _tankSize * 2, .y = _tankSize * 2}, Author::Enemy1, Direction::DOWN)};

	EXPECT_EQ(enemyBot->GetHealth(), 100);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(enemyBot->GetHealth(), 100);
}

// a shot tank bonus hands out no life
TEST_F(BonusTestDestroy, TankDestroyNoExtraLife)
{
	unsigned short respawnActual{3u};
	auto respawnSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		respawnActual = event.respawnCount;
	})};

	CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = 7.0, .w = _tankSize, .h = _tankSize}, BonusType::Tank);

	const unsigned short playerSpawnCount{respawnActual};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(playerSpawnCount, respawnActual);

}

// a shot star raises no tier
TEST_F(BonusTestDestroy, StarDestroyTierRemainTheSame)
{
	const auto player{CreatePlayer({.x = 0.0, .y = _tankSize * 3.0}, Author::Player2)};
	CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = 7.0, .w = _tankSize, .h = _tankSize}, BonusType::Star);

	EXPECT_EQ(player->GetTier(), 1u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(player->GetTier(), 1u);
}

// and a shot shovel leaves the eagle's wall brick
TEST_F(BonusTestDestroy, ShovelNotPickUpByPlayerThenfortressWallRemainTheSame)
{
	CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = 7.0, .w = _tankSize, .h = _tankSize}, BonusType::Shovel);
	const ObjRectangle fortressRect{.x = _tankSize + 1.0, .y = 0, .w = _gridSize, .h = _gridSize};
	_events->EmitEvent(SpawnObstacleEvent{.rect = fortressRect, .type = ObstacleType::Fortress});

	EXPECT_NE(dynamic_cast<FortressBrickWall*>(_fortressWall.get()), nullptr);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_NE(dynamic_cast<FortressBrickWall*>(_fortressWall.get()), nullptr);
}
