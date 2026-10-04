#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/TankPool.h"
#include "components/EventSystem.h"
#include "components/events/InputEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/pawns/Tank.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/InputChannel.h"
#include "geometry/ObjRectangle.h"
#include "gtest/gtest.h"
#include <memory>
#include <vector>

// the client never sees a pickup land - it learns of the heal only from the health the host reports
class TankHealTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<TankPool> _tankPool{nullptr};
	std::unique_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::vector<EventSubscription> _instantSpawnAnimationSubs{};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	std::vector<HealthChangedEvent> _reportedHealth;
	EventSubscription _spawnQueueSub{};
	EventSubscription _healthSub{};
	double _deltaTimeOneFrame{1.0 / 60.0};
	int _tankHealth{100};

	void SetUp() override
	{
		_gameConfig.gameMode = GameMode::PlayAsHost;
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_tankPool = std::make_shared<TankPool>(_events, _allObjects, _gameConfig, _bulletPool);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, _allObjects, _gameConfig);
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
		_healthSub = _events->AddListener([this](const HealthChangedEvent& event)
		{
			_reportedHealth.push_back(event);
		});
	}

	std::shared_ptr<Tank> CreatePlayer(const unsigned short tier = 1u)
	{
		const ObjRectangle rect{.x = 0.0, .y = 0.0, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};
		auto player{TestUtils::CreatePlayer(rect, _tankHealth, Author::Player1, _allObjects, _events, Direction::DOWN,
											_tankPool, _gameConfig, tier)};

		return player;
	}

	void DriveIntoBonus(const BonusType type)
	{
		const double tankSize{_gameConfig.tankSize};
		constexpr bool isPressed{true};
		_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});
		_bonusSpawner->SpawnBonus({.x = 0.0, .y = tankSize + 1.0, .w = tankSize, .h = tankSize}, type);

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	}
};

TEST_F(TankHealTest, APickupHealIsReported)
{
	const auto player{CreatePlayer()};
	player->TakeDamage(1u, Author::Enemy1);
	_reportedHealth.clear();

	DriveIntoBonus(BonusType::Helmet);

	ASSERT_FALSE(_reportedHealth.empty());
	EXPECT_EQ(_reportedHealth.back().health, player->GetHealth());
	EXPECT_EQ(_reportedHealth.back().uuid, player->GetUuid());
}

//NOTE: a maxed tank returns early from the star handler - the heal must not hang on the tier going up
TEST_F(TankHealTest, AMaxedTankStillHealsOnAStar)
{
	const auto player{CreatePlayer(7u)};
	const int healthBefore{player->GetHealth()};

	DriveIntoBonus(BonusType::Star);

	EXPECT_GT(player->GetHealth(), healthBefore);
	ASSERT_FALSE(_reportedHealth.empty());
	EXPECT_EQ(_reportedHealth.back().health, player->GetHealth());
}
