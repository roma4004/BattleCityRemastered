#include "TestUtils.h"
#include "components/ObstacleSpawner.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/TankPool.h"
#include "components/EventSystem.h"
#include "components/events/InputEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/StatisticsEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/BaseObj.h"
#include "entities/bonuses/Bonus.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Tank.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/InputChannel.h"
#include "gtest/gtest.h"
#include <memory>
#include <vector>

//NOTE: a dead object is still in _allObjects until PostTickUpdate, so within one tick everything
//walking the container meets corpses - these check that nothing acts on one twice
class DeadObjectFilterTest : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<TankPool> _tankPool{nullptr};
	std::unique_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::vector<EventSubscription> _instantSpawnAnimationSubs{};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	BulletCaliber _caliber{.speed = 300.0, .damage = 1u, .damageRadius = 12.0, .tier = 1u, .size{.x = 6.0, .y = 5.0}};
	Uuid _uuid{};
	double _tankSize{};
	double _deltaTimeOneFrame{1.0 / 60.0};
	int _tankHealth{1};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_obstacleSpawner = std::make_unique<ObstacleSpawner>(_events, _gameConfig);
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_tankPool = std::make_shared<TankPool>(_events, _allObjects, _gameConfig, _bulletPool);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, _allObjects, _gameConfig);
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
		_tankSize = _gameConfig.tankSize;

		_allObjects.reserve(8);
	}

	std::shared_ptr<Tank> CreatePlayer(const FPoint pos, const Author author, const Direction dir, const int health)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};
		auto player{TestUtils::CreatePlayer(rect, health, author, _allObjects, _events, dir, _tankPool, _gameConfig)};

		return player;
	}

	std::shared_ptr<Bullet> CreateBullet(const FPoint pos, const Direction dir, const Author author)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _caliber.size.x, .h = _caliber.size.y};
		auto bullet{TestUtils::CreateBullet(rect, 1, _bulletPool, _events, _caliber, dir,
											author)};

		return bullet;
	}

	std::shared_ptr<BaseObj> SpawnObstacle(const FPoint pos, const ObstacleType type) const
	{
		return TestUtils::SpawnObstacle(_events, _allObjects, pos, type, _gameConfig);
	}
};

// two players step onto the same bonus in one tick: it dies once and reports a single pickup, not
// one per tank that reached it
TEST_F(DeadObjectFilterTest, BonusIsPickedUpOncePerFrame)
{
	int pickups{};
	const EventSubscription pickupSub{_events->AddListener([&pickups](const StatisticsBonusPickupEvent&)
	{
		++pickups;
	})};

	const ObjRectangle bonusRect{.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize};
	_bonusSpawner->SpawnBonus(bonusRect, BonusType::Timer);
	auto* bonus{dynamic_cast<Bonus*>(_allObjects.back().get())};
	ASSERT_NE(nullptr, bonus);

	CreatePlayer({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN, _gameConfig.tankHealth);
	CreatePlayer({.x = 0.0, .y = _tankSize * 2.0 + 2.0}, Author::Player2, Direction::UP, _gameConfig.tankHealth);

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});
	_events->EmitEvent(Key(InputChannel::LocalP2), MoveUpEvent{.isPressed = isPressed});

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(bonus->GetIsAlive());
	EXPECT_EQ(1, pickups);
}

// a one-cell wall takes a bullet from each side in the same tick: one death event, health stops at
// zero instead of going negative
TEST_F(DeadObjectFilterTest, BrickWallHitByTwoBulletsDiesOnce)
{
	int deaths{};
	const EventSubscription deathSub{_events->AddListener([&deaths](const BrickWallDiedEvent&) { ++deaths; })};

	const double cell{_gameConfig.gridOffset};
	const auto wall{SpawnObstacle(FPoint{.x = 100.0, .y = 100.0}, ObstacleType::Brick)};
	_caliber.damage = static_cast<unsigned int>(wall->GetHealth());

	CreateBullet({.x = 100.0 - _caliber.size.x, .y = 103.0}, Direction::RIGHT, Author::Player1);
	CreateBullet({.x = 100.0 + cell, .y = 103.0}, Direction::LEFT, Author::Player1);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(wall->GetIsAlive());
	EXPECT_EQ(0, wall->GetHealth());
	EXPECT_EQ(1, deaths);
}

// two enemy bullets reach a one-health player in the same tick: he dies once, the second bullet finds
// a corpse and is not counted
TEST_F(DeadObjectFilterTest, TankKilledThisFrameTakesNoSecondHit)
{
	int deaths{};
	const EventSubscription deathSub{_events->AddListener([&deaths](const TankDiedEvent&) { ++deaths; })};

	const auto player{CreatePlayer({.x = 100.0, .y = 100.0}, Author::Player1, Direction::UP, _tankHealth)};

	CreateBullet({.x = 100.0 - _caliber.size.x, .y = 115.0}, Direction::RIGHT, Author::Enemy1);
	CreateBullet({.x = 100.0 + _tankSize, .y = 115.0}, Direction::LEFT, Author::Enemy2);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(player->GetIsAlive());
	EXPECT_EQ(0, player->GetHealth());
	EXPECT_EQ(1, deaths);
}
