#include "TestUtils.h"
#include "application/GameConfig.h"
#include "application/ProjectConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/TankSpawner.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/TimingEvents.h"
#include "components/managers/RespawnManager.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Player.h"
#include "enums/Direction.h"
#include "enums/Faction.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include <memory>
#include <optional>

//NOTE: no AnimationManager here on purpose - the point is that the game logic emits the right event
//with the right data, not that the manager then draws something
class AnimationTriggersTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	ProjectConfig _projectConfig{"", true};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::vector<EventSubscription> _subs{};
	EventSubscription _spawnQueueSub{};

	std::optional<AnimationCreateBulletExplosionEvent> _bulletExplosion{};
	std::optional<AnimationCreateTankExplosionEvent> _tankExplosion{};
	std::optional<AnimationCreateTankSpawnEvent> _tankSpawn{};
	std::optional<AnimationCreateWaterEvent> _water{};
	std::optional<AnimationBonusHelmetChangeEvent> _helmet{};

	BulletCalibre _calibre{.speed = 300.0, .damage = 1u, .damageRadius = 12.0, .tier = 1u, .size{.x = 6.0, .y = 5.0}};
	Uuid _uuid{};
	unsigned short _health{1};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, &_allObjects);
		_allObjects.reserve(8u);
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _gameConfig);

		_subs.push_back(_events->AddListener([this](const AnimationCreateBulletExplosionEvent& event)
		{
			_bulletExplosion = event;
		}));
		_subs.push_back(_events->AddListener([this](const AnimationCreateTankExplosionEvent& event)
		{
			_tankExplosion = event;
		}));
		_subs.push_back(_events->AddListener([this](const AnimationCreateTankSpawnEvent& event)
		{
			_tankSpawn = event;
		}));
		_subs.push_back(_events->AddListener([this](const AnimationCreateWaterEvent& event) { _water = event; }));
		_subs.push_back(_events->AddListener([this](const AnimationBonusHelmetChangeEvent& event)
		{
			_helmet = event;
		}));
	}
};

TEST_F(AnimationTriggersTest, BulletExplodesWhereItHit)
{
	const ObjRectangle bulletRect{.x = 0.0, .y = 0.0, .w = _calibre.size.x, .h = _calibre.size.y};
	auto bullet = TestUtils::CreateBullet(bulletRect, _health, _uuid, "Bullet1", Faction::PlayerTeam, &_allObjects,
										  _events, _calibre, Direction::DOWN, GameMode::OnePlayer, _gameConfig,
										  "Player1");
	_allObjects.emplace_back(bullet);
	_allObjects.emplace_back(std::make_shared<BrickWall>(ObjRectangle{.x = 0.0, .y = 8.0, .w = 12.0, .h = 12.0},
														_events, _uuid, GameMode::OnePlayer));

	_events->EmitEvent(TickUpdateEvent{.deltaTime = 1.0 / 60.0});

	ASSERT_TRUE(_bulletExplosion.has_value());
	EXPECT_EQ(_bulletExplosion->name, bullet->GetName());
	EXPECT_EQ(_bulletExplosion->rect.y, bullet->GetRect().y);
}

//NOTE: the client never runs the collision itself - the burst has to come from the host's despawn
TEST_F(AnimationTriggersTest, ClientBulletExplodesOnDespawn)
{
	const ObjRectangle bulletRect{.x = 20.0, .y = 30.0, .w = _calibre.size.x, .h = _calibre.size.y};
	auto bullet = TestUtils::CreateBullet(bulletRect, _health, _uuid, "Bullet1", Faction::PlayerTeam, &_allObjects,
										  _events, _calibre, Direction::DOWN, GameMode::PlayAsClient, _gameConfig,
										  "Player1");
	_allObjects.emplace_back(bullet);

	_events->EmitEvent(Key(bullet->GetUuid()), DespawnedEvent{.who = "Bullet1", .uuid = bullet->GetUuid()});

	ASSERT_TRUE(_bulletExplosion.has_value());
	EXPECT_EQ(_bulletExplosion->name, bullet->GetName());
	EXPECT_EQ(_bulletExplosion->rect.x, bulletRect.x);
	EXPECT_EQ(_bulletExplosion->rect.y, bulletRect.y);
}

//NOTE: the burst comes from the death, not from the destructor - a field wiped on reset must not
//explode
TEST_F(AnimationTriggersTest, TankExplodesWhereItDied)
{
	constexpr ObjRectangle tankRect{.x = 40.0, .y = 50.0, .w = 12.0, .h = 12.0};
	auto tank = TestUtils::CreateTank<Player>(tankRect, _health, _uuid, "Player1", Faction::PlayerTeam,
											  &_allObjects, _events, 1u, 142.0, Direction::UP,
											  GameMode::OnePlayer, _bulletPool, _gameConfig);

	tank->TakeDamage(static_cast<unsigned int>(tank->GetHealth()), "Enemy1", Faction::EnemyTeam);

	ASSERT_TRUE(_tankExplosion.has_value());
	EXPECT_EQ(_tankExplosion->name, "Player1");
	EXPECT_EQ(_tankExplosion->rect.x, tankRect.x);
	EXPECT_EQ(_tankExplosion->rect.y, tankRect.y);
}

TEST_F(AnimationTriggersTest, ALiveTankTakenOffTheFieldExplodesNothing)
{
	{
		auto tank = TestUtils::CreateTank<Player>(ObjRectangle{.x = 0.0, .y = 0.0, .w = 12.0, .h = 12.0}, _health,
												  _uuid, "Player1", Faction::PlayerTeam, &_allObjects, _events, 1u,
												  142.0, Direction::UP, GameMode::OnePlayer, _bulletPool,
												  _gameConfig);
	}

	EXPECT_FALSE(_tankExplosion.has_value());
}

TEST_F(AnimationTriggersTest, SpawnedTankAsksForItsSpawnBurst)
{
	std::shared_ptr<RespawnManager> respawnManager{nullptr};
	std::shared_ptr<TankSpawner> tankSpawner{nullptr};
	TestUtils::ApplyGameMode(_events, &_allObjects, _gameConfig, GameMode::OnePlayer, respawnManager, tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});

	ASSERT_TRUE(_tankSpawn.has_value());
	EXPECT_FALSE(_tankSpawn->name.empty());
	EXPECT_NE(_tankSpawn->uuid, Uuid{});
}

TEST_F(AnimationTriggersTest, WaterTileAsksForItsFlowWhenBuilt)
{
	constexpr ObjRectangle waterRect{.x = 24.0, .y = 36.0, .w = 12.0, .h = 12.0};
	const WaterTile water{waterRect, _events, _uuid, GameMode::OnePlayer};

	ASSERT_TRUE(_water.has_value());
	EXPECT_EQ(_water->rect.x, waterRect.x);
	EXPECT_EQ(_water->rect.y, waterRect.y);
}

TEST_F(AnimationTriggersTest, HelmetPickupTurnsTheShieldOnAndOff)
{
	auto tank = TestUtils::CreateTank<Player>(ObjRectangle{.x = 0.0, .y = 0.0, .w = 12.0, .h = 12.0}, _health, _uuid,
											  "Player1", Faction::PlayerTeam, &_allObjects, _events, 1u, 142.0,
											  Direction::UP, GameMode::OnePlayer, _bulletPool, _gameConfig);

	_events->EmitEvent(Key(std::string{"Player1"}), BonusHelmetStatusChangeEvent{.isActive = true});

	ASSERT_TRUE(_helmet.has_value());
	EXPECT_EQ(_helmet->name, "Player1");
	EXPECT_TRUE(_helmet->isEnable);

	_events->EmitEvent(Key(std::string{"Player1"}), BonusHelmetStatusChangeEvent{.isActive = false});

	EXPECT_FALSE(_helmet->isEnable);
}
