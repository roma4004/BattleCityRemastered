#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/SpawnEvents.h"
#include "entities/pawns/Tank.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/DespawnReason.h"
#include "enums/Faction.h"
#include "enums/GameMode.h"
#include "geometry/ObjRectangle.h"
#include "utils/UuidUtils.h"
#include "gtest/gtest.h"
#include <memory>
#include <vector>

class ClientMirrorTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::unique_ptr<BonusSpawner> _bonusSpawner{nullptr};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	std::vector<HealthChangedEvent> _reportedHealth;
	EventSubscription _spawnQueueSub{};
	EventSubscription _healthSub{};
	ObjRectangle _tankRect{.x = 100.0, .y = 100.0, .w = 36.0, .h = 36.0};
	FPoint _bonusPos{.x = 200.0, .y = 200.0};
	int _tankHealth{100};
	int _bonusHeal{50};

	void SetUp() override
	{
		_gameConfig.gameMode = GameMode::PlayAsClient;
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, _allObjects, _gameConfig);
		_healthSub = _events->AddListener([this](const HealthChangedEvent& event)
		{
			_reportedHealth.push_back(event);
		});
	}
};

//NOTE: the host sends health as an absolute value - applying the heal here too would land it twice
TEST_F(ClientMirrorTest, AClientTakesHealthOffTheWireInsteadOfHealingItself)
{
	const std::shared_ptr<Tank> enemy =
			TestUtils::CreateBot(
					_tankRect, _tankHealth, UuidUtils::GetRandomUuid(), Author::Enemy1, Faction::EnemyTeam,
					_allObjects, _events, 1u, _gameConfig.tankSpeed, Direction::UP, _gameConfig.gameMode, _bulletPool,
					_gameConfig);

	_events->EmitEvent(Key(Author::Enemy1), BonusStarPickupEvent{});

	EXPECT_EQ(enemy->GetHealth(), _tankHealth);
	EXPECT_TRUE(_reportedHealth.empty());

	_events->EmitEvent(Key(enemy->GetUuid()),
					   HealthChangedEvent{.health = _tankHealth + _bonusHeal, .uuid = enemy->GetUuid()});

	EXPECT_EQ(enemy->GetHealth(), _tankHealth + _bonusHeal);
}

//NOTE: the burst only draws here - what puts the bonus on the field is the host saying it settled
TEST_F(ClientMirrorTest, ABonusLandsWhenTheHostSaysItSettled)
{
	const Uuid uuid = UuidUtils::GetRandomUuid();

	_events->EmitEvent(BonusSpawnedEvent{.pos = _bonusPos, .type = BonusType::Star, .uuid = uuid, .isSuper = false});
	_events->EmitEvent(SpawnAnimationFinishedEvent{.uuid = uuid});

	EXPECT_TRUE(_allObjects.empty());

	_events->EmitEvent(BonusSpawnCompletedEvent{.uuid = uuid});

	EXPECT_EQ(_allObjects.size(), 1u);
}

//NOTE: a bonus picked up mid-burst is never completed by the host, so no ghost is left behind
TEST_F(ClientMirrorTest, ABonusRetiredDuringItsBurstNeverLands)
{
	const Uuid uuid = UuidUtils::GetRandomUuid();

	_events->EmitEvent(BonusSpawnedEvent{.pos = _bonusPos, .type = BonusType::Star, .uuid = uuid, .isSuper = false});
	_events->EmitEvent(Key(uuid), DespawnedEvent{.uuid = uuid, .reason = DespawnReason::PickedUp});
	_events->EmitEvent(SpawnAnimationFinishedEvent{.uuid = uuid});

	EXPECT_TRUE(_allObjects.empty());
}

TEST_F(ClientMirrorTest, CompletingOneBonusLeavesTheOtherPending)
{
	const Uuid settled = UuidUtils::GetRandomUuid();
	const Uuid pending = UuidUtils::GetRandomUuid();

	_events->EmitEvent(BonusSpawnedEvent{.pos = _bonusPos, .type = BonusType::Star, .uuid = settled, .isSuper = false});
	_events->EmitEvent(BonusSpawnedEvent{.pos = _bonusPos, .type = BonusType::Star, .uuid = pending, .isSuper = false});
	_events->EmitEvent(BonusSpawnCompletedEvent{.uuid = settled});

	EXPECT_EQ(_allObjects.size(), 1u);
}
