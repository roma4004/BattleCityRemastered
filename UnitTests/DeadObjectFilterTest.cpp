#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
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
#include "enums/Faction.h"
#include "enums/GameMode.h"
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
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::unique_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::vector<EventSubscription> _instantSpawnAnimationSubs{};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	BulletCalibre _calibre{.speed = 300.0, .damage = 1u, .damageRadius = 12.0, .tier = 1u, .size{.x = 6.0, .y = 5.0}};
	Uuid _uuid{};
	double _tankSize{};
	double _tankSpeed{142.0};
	double _deltaTimeOneFrame{1.0 / 60.0};
	int _tankHealth{1};
	GameMode _gameMode{GameMode::OnePlayer};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, _allObjects, _gameConfig);
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
		_tankSize = _gameConfig.tankSize;

		_allObjects.reserve(8);
	}
};

// Two tanks reach the same bonus within one tick - only the first one gets it
TEST_F(DeadObjectFilterTest, BonusIsPickedUpOncePerFrame)
{
	int pickups{0};
	const EventSubscription pickupSub = _events->AddListener([&pickups](const StatisticsBonusPickupEvent&)
	{
		++pickups;
	});

	const ObjRectangle bonusRect{.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize};
	_bonusSpawner->SpawnBonus(bonusRect, BonusType::Timer);
	auto* bonus = dynamic_cast<Bonus*>(_allObjects.back().get());
	ASSERT_NE(nullptr, bonus);

	std::shared_ptr<Tank> playerAbove = TestUtils::CreatePlayer(
			ObjRectangle{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize}, _gameConfig.tankHealth, _uuid,
			Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u, _tankSpeed, Direction::DOWN, _bulletPool,
			_gameConfig);
	_allObjects.emplace_back(playerAbove);

	std::shared_ptr<Tank> playerBelow = TestUtils::CreatePlayer(
			ObjRectangle{.x = 0.0, .y = _tankSize * 2.0 + 2.0, .w = _tankSize, .h = _tankSize}, _gameConfig.tankHealth,
			_uuid, Author::Player2, Faction::PlayerTeam, _allObjects, _events, 1u, _tankSpeed, Direction::UP,
			_bulletPool, _gameConfig);
	_allObjects.emplace_back(playerBelow);

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});
	_events->EmitEvent(Key(InputChannel::LocalP2), MoveUpEvent{.isPressed = isPressed});

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(bonus->GetIsAlive());
	EXPECT_EQ(1, pickups);
}

// Two bullets land on the same brick wall within one tick - it dies once, not into negative health
TEST_F(DeadObjectFilterTest, BrickWallHitByTwoBulletsDiesOnce)
{
	int deaths{0};
	const EventSubscription deathSub = _events->AddListener([&deaths](const BrickWallDiedEvent&) { ++deaths; });

	const double cell = _gameConfig.gridOffset;
	auto wall = std::make_shared<BrickWall>(ObjRectangle{.x = 100.0, .y = 100.0, .w = cell, .h = cell}, _events,
											_uuid, _gameMode);
	_allObjects.emplace_back(wall);

	const ObjRectangle fromLeft{.x = 100.0 - _calibre.size.x, .y = 103.0, .w = _calibre.size.x, .h = _calibre.size.y};
	auto bullet1{
			TestUtils::CreateBullet(
					fromLeft, 1, _uuid, Faction::PlayerTeam, _allObjects, _events, _calibre, Direction::RIGHT,
					_gameConfig, Author::Player1)
	};
	_allObjects.emplace_back(bullet1);

	const ObjRectangle fromRight{.x = 100.0 + cell, .y = 103.0, .w = _calibre.size.x, .h = _calibre.size.y};
	auto bullet2{
			TestUtils::CreateBullet(
					fromRight, 1, _uuid, Faction::PlayerTeam, _allObjects, _events, _calibre, Direction::LEFT,
					_gameConfig, Author::Player1)
	};
	_allObjects.emplace_back(bullet2);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(wall->GetIsAlive());
	EXPECT_EQ(0, wall->GetHealth());
	EXPECT_EQ(1, deaths);
}

// The second bullet arrives after the tank died earlier in the same tick - it must not kill it again
TEST_F(DeadObjectFilterTest, TankKilledThisFrameTakesNoSecondHit)
{
	int deaths{0};
	const EventSubscription deathSub = _events->AddListener([&deaths](const TankDiedEvent&) { ++deaths; });

	std::shared_ptr<Tank> player = TestUtils::CreatePlayer(
			ObjRectangle{.x = 100.0, .y = 100.0, .w = _tankSize, .h = _tankSize}, _tankHealth, _uuid, Author::Player1,
			Faction::PlayerTeam, _allObjects, _events, 1u, _tankSpeed, Direction::UP, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const ObjRectangle fromLeft{.x = 100.0 - _calibre.size.x, .y = 115.0, .w = _calibre.size.x, .h = _calibre.size.y};
	auto bullet1{
			TestUtils::CreateBullet(
					fromLeft, 1, _uuid, Faction::EnemyTeam, _allObjects, _events, _calibre, Direction::RIGHT,
					_gameConfig, Author::Enemy1)
	};
	_allObjects.emplace_back(bullet1);

	const ObjRectangle fromRight{.x = 100.0 + _tankSize, .y = 115.0, .w = _calibre.size.x, .h = _calibre.size.y};
	auto bullet2{
			TestUtils::CreateBullet(
					fromRight, 1, _uuid, Faction::EnemyTeam, _allObjects, _events, _calibre, Direction::LEFT,
					_gameConfig, Author::Enemy2)
	};
	_allObjects.emplace_back(bullet2);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(player->GetIsAlive());
	EXPECT_EQ(0, player->GetHealth());
	EXPECT_EQ(1, deaths);
}
