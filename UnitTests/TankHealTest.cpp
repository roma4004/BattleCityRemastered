#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/TankPool.h"
#include "components/EventSystem.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/ReplicationEvents.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "geometry/ObjRectangle.h"
#include "gtest/gtest.h"
#include <memory>
#include <vector>

class TankHealTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<TankPool> _tankPool{nullptr};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	std::vector<HealthChangedEvent> _reportedHealth;
	EventSubscription _spawnQueueSub{};
	EventSubscription _healthSub{};
	int _tankHealth{100};

	void SetUp() override
	{
		_gameConfig.gameMode = GameMode::PlayAsHost;
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_tankPool = std::make_shared<TankPool>(_events, _allObjects, _gameConfig, _bulletPool);
		_healthSub = _events->AddListener([this](const HealthChangedEvent& event)
		{
			_reportedHealth.push_back(event);
		});
	}

	std::shared_ptr<Tank> CreateBot(const FPoint pos, const Author author, const unsigned short tier = 1u)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};
		auto bot{TestUtils::CreateBot(rect, _tankHealth, author, _allObjects, _events, Direction::UP, _tankPool,
									  _gameConfig, tier)};

		return bot;
	}
};

//NOTE: a maxed tank returns early from the star handler, so the heal cannot ride the pickup event
TEST_F(TankHealTest, AMaxedTankStillReportsTheHealth)
{
	const auto enemy{CreateBot({.x = 100.0, .y = 100.0}, Author::Enemy1, 4u)};

	_events->EmitEvent(Key(Author::Enemy1), BonusStarPickupEvent{});

	EXPECT_GT(enemy->GetHealth(), _tankHealth);
	ASSERT_EQ(_reportedHealth.size(), 1u);
	EXPECT_EQ(_reportedHealth.front().health, enemy->GetHealth());
	EXPECT_EQ(_reportedHealth.front().uuid, enemy->GetUuid());
}

TEST_F(TankHealTest, AnUpgradingTankReportsTheHealthToo)
{
	const auto enemy{CreateBot({.x = 100.0, .y = 100.0}, Author::Enemy2)};

	_events->EmitEvent(Key(Author::Enemy2), BonusStarPickupEvent{});

	EXPECT_GT(enemy->GetHealth(), _tankHealth);
	ASSERT_EQ(_reportedHealth.size(), 1u);
	EXPECT_EQ(_reportedHealth.front().health, enemy->GetHealth());
}
