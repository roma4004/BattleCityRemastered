#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/ReplicationEvents.h"
#include "entities/pawns/Bot.h"
#include "enums/Direction.h"
#include "enums/Faction.h"
#include "enums/GameMode.h"
#include "geometry/ObjRectangle.h"
#include "utils/UuidUtils.h"
#include "gtest/gtest.h"
#include <memory>
#include <vector>

class TankHealTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	std::vector<HealthChangedEvent> _reportedHealth;
	EventSubscription _spawnQueueSub{};
	EventSubscription _healthSub{};
	ObjRectangle _tankRect{.x = 100.0, .y = 100.0, .w = 36.0, .h = 36.0};
	int _tankHealth{100};
	int _bonusHeal{50};

	void SetUp() override
	{
		_gameConfig.gameMode = GameMode::PlayAsHost;
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_healthSub = _events->AddListener([this](const HealthChangedEvent& event)
		{
			_reportedHealth.push_back(event);
		});
	}
};

//NOTE: a maxed tank returns early from the star handler, so the heal cannot ride the pickup event
TEST_F(TankHealTest, AMaxedTankStillReportsTheHealth)
{
	const std::shared_ptr<Bot> enemy =
			TestUtils::CreateTank<Bot>(
					_tankRect, _tankHealth, UuidUtils::GetRandomUuid(), Author::Enemy1, Faction::EnemyTeam, _allObjects,
					_events, 4u, _gameConfig.tankSpeed, Direction::UP, _gameConfig.gameMode, _bulletPool, _gameConfig);

	_events->EmitEvent(Key(Author::Enemy1), BonusStarPickupEvent{});

	EXPECT_EQ(enemy->GetHealth(), _tankHealth + _bonusHeal);
	ASSERT_EQ(_reportedHealth.size(), 1u);
	EXPECT_EQ(_reportedHealth.front().health, _tankHealth + _bonusHeal);
	EXPECT_EQ(_reportedHealth.front().uuid, enemy->GetUuid());
}

TEST_F(TankHealTest, AnUpgradingTankReportsTheHealthToo)
{
	const std::shared_ptr<Bot> enemy =
			TestUtils::CreateTank<Bot>(
					_tankRect, _tankHealth, UuidUtils::GetRandomUuid(), Author::Enemy2, Faction::EnemyTeam, _allObjects,
					_events, 1u, _gameConfig.tankSpeed, Direction::UP, _gameConfig.gameMode, _bulletPool, _gameConfig);

	_events->EmitEvent(Key(Author::Enemy2), BonusStarPickupEvent{});

	EXPECT_EQ(enemy->GetHealth(), _tankHealth + _bonusHeal);
	ASSERT_EQ(_reportedHealth.size(), 1u);
	EXPECT_EQ(_reportedHealth.front().health, _tankHealth + _bonusHeal);
}
