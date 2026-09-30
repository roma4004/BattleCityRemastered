#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/TankPool.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/SpawnEvents.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Tank.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/DespawnReason.h"
#include "enums/GameMode.h"
#include "geometry/ObjRectangle.h"
#include "utils/UuidUtils.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <memory>
#include <optional>
#include <vector>

class ClientMirrorTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<TankPool> _tankPool{nullptr};
	std::unique_ptr<BonusSpawner> _bonusSpawner{nullptr};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	std::vector<HealthChangedEvent> _reportedHealth;
	EventSubscription _spawnQueueSub{};
	EventSubscription _healthSub{};
	FPoint _bonusPos{.x = 200.0, .y = 200.0};
	int _tankHealth{100};
	int _bonusHeal{50};

	void SetUp() override
	{
		_gameConfig.gameMode = GameMode::PlayAsClient;
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_tankPool = std::make_shared<TankPool>(_events, _allObjects, _gameConfig, _bulletPool);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, _allObjects, _gameConfig);
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

//NOTE: the server sends health as an absolute value - applying the heal here too would land it twice
TEST_F(ClientMirrorTest, AClientTakesHealthOffTheWireInsteadOfHealingItself)
{
	const auto enemy{CreateBot({.x = 100.0, .y = 100.0}, Author::Enemy1)};

	_events->EmitEvent(Key(Author::Enemy1), BonusStarPickupEvent{});

	EXPECT_EQ(enemy->GetHealth(), _tankHealth);
	EXPECT_TRUE(_reportedHealth.empty());

	_events->EmitEvent(Key(enemy->GetUuid()),
					   HealthChangedEvent{.health = _tankHealth + _bonusHeal, .uuid = enemy->GetUuid()});

	EXPECT_EQ(enemy->GetHealth(), _tankHealth + _bonusHeal);
}

//NOTE: the burst only draws here - what puts the bonus on the field is the server saying it settled
TEST_F(ClientMirrorTest, ABonusLandsWhenTheServerSaysItSettled)
{
	const Uuid uuid{UuidUtils::GetRandomUuid()};

	_events->EmitEvent(BonusSpawnedEvent{.pos = _bonusPos, .type = BonusType::Star, .uuid = uuid, .isSuper = false});
	_events->EmitEvent(SpawnAnimationFinishedEvent{.uuid = uuid});

	EXPECT_TRUE(_allObjects.empty());

	_events->EmitEvent(BonusSpawnCompletedEvent{.uuid = uuid});

	EXPECT_EQ(_allObjects.size(), 1u);
}

//NOTE: the server's loop may run slower than ours - a burst of our own would end before the bonus lands
TEST_F(ClientMirrorTest, AClientBonusBurstWaitsForTheServer)
{
	std::optional<AnimationCreateBonusSpawnEvent> burst{};
	const EventSubscription burstSub{_events->AddListener(
			[&burst](const AnimationCreateBonusSpawnEvent& event) { burst = event; })};

	const Uuid uuid{UuidUtils::GetRandomUuid()};
	_events->EmitEvent(BonusSpawnedEvent{.pos = _bonusPos, .type = BonusType::Star, .uuid = uuid, .isSuper = false});

	ASSERT_TRUE(burst.has_value());
	EXPECT_TRUE(burst->isEndless);
}

//NOTE: a bonus picked up mid-burst is never completed by the server, so no ghost is left behind
TEST_F(ClientMirrorTest, ABonusRetiredDuringItsBurstNeverLands)
{
	const Uuid uuid{UuidUtils::GetRandomUuid()};

	_events->EmitEvent(BonusSpawnedEvent{.pos = _bonusPos, .type = BonusType::Star, .uuid = uuid, .isSuper = false});
	_events->EmitEvent(Key(uuid), DespawnedEvent{.uuid = uuid, .reason = DespawnReason::PickedUp});
	_events->EmitEvent(SpawnAnimationFinishedEvent{.uuid = uuid});

	EXPECT_TRUE(_allObjects.empty());
}

// two bonuses bursting at once: the host's word lands one of them and leaves the other waiting
TEST_F(ClientMirrorTest, CompletingOneBonusLeavesTheOtherPending)
{
	const Uuid settled{UuidUtils::GetRandomUuid()};
	const Uuid pending{UuidUtils::GetRandomUuid()};

	_events->EmitEvent(BonusSpawnedEvent{.pos = _bonusPos, .type = BonusType::Star, .uuid = settled, .isSuper = false});
	_events->EmitEvent(BonusSpawnedEvent{.pos = _bonusPos, .type = BonusType::Star, .uuid = pending, .isSuper = false});
	_events->EmitEvent(BonusSpawnCompletedEvent{.uuid = settled});

	EXPECT_EQ(_allObjects.size(), 1u);
}

TEST_F(ClientMirrorTest, AMirroredShotKeepsTheDamageTheHostRolled)
{
	CreateBot({.x = 100.0, .y = 100.0}, Author::Enemy1);
	constexpr unsigned int hostRolled{7u};

	_events->EmitEvent(Key(Author::Enemy1), TankShotEvent{.who = Author::Enemy1,
														  .dir = Direction::UP,
														  .bulletUuid = UuidUtils::GetRandomUuid(),
														  .damage = hostRolled});

	const auto isBullet = [](const std::shared_ptr<BaseObj>& object)
	{
		return std::dynamic_pointer_cast<Bullet>(object) != nullptr;
	};
	const auto shell{std::ranges::find_if(_allObjects, isBullet)};
	ASSERT_NE(shell, _allObjects.end());

	EXPECT_EQ(std::dynamic_pointer_cast<Bullet>(*shell)->GetDamage(), hostRolled);
}
