#include "TestUtils.h"
#include "components/BulletPool.h"
#include "application/GameConfig.h"
#include "components/EventSystem.h"
#include "components/events/TimingEvents.h"
#include "components/GameStatistics.h"
#include "components/StatisticsData.h"
#include "entities/pawns/Bullet.h"
#include "enums/Direction.h"
#include "gtest/gtest.h"
#include <memory>

// every test here lets the player's own bullet, fired downwards in SetUp, meet one flying up at it in
// the same tick, and reads who got the hit counted; what differs is who shot the incoming one
class StatisticsTestAdvanced : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<GameStatistics> _statistics{nullptr};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.0 / 60.0};
	double _tankSize{};
	unsigned short _bulletHealth{1u};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_statistics = std::make_shared<GameStatistics>(_events);
		_tankSize = _gameConfig.tankSize;

		CreateBullet({.x = 0.0, .y = 5.0}, Direction::DOWN, 1u, Author::Player1);
	}

	void TearDown() override {}

	void CreateBullet(const FPoint pos, const Direction dir, const unsigned short tier, const Author author)
	{
		const BulletCaliber caliber{.speed = 300.0,
									.damage = 1u,
									.damageRadius = 12.0,
									.tier = tier,
									.size{.x = 6.0, .y = 5.0}};
		const ObjRectangle rectBullet{.x = pos.x, .y = pos.y, .w = caliber.size.x, .h = caliber.size.y};
		std::ignore = TestUtils::CreateBullet(rectBullet, _bulletHealth, _bulletPool, _events, caliber, dir, author);
	}
};

// the incoming one is an enemy's
TEST_F(StatisticsTestAdvanced, BulletHitByEnemyBullet)
{
	CreateBullet({.x = 0.0, .y = 5.0 + 1}, Direction::UP, 1u, Author::Enemy1);

	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bulletHitByEnemy, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerOne, 1u);
	EXPECT_EQ(_statistics->GetData().bulletHitByEnemy, 1u);
}

// the incoming one is player two's - friendly fire counts the same
TEST_F(StatisticsTestAdvanced, BulletHitByPlayerOne)
{
	CreateBullet({.x = 0.0, .y = 5.0 + 1}, Direction::UP, 1u, Author::Player2);

	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerOne, 1u);
	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerTwo, 1u);
}
