#include "TestUtils.h"
#include "application/GameConfig.h"
#include "application/ProjectConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/TankSpawner.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/events/TimingEvents.h"
#include "components/managers/AnimationManager.h"
#include "components/managers/RespawnManager.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Tank.h"
#include "enums/AnimationType.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "utils/Uuid.h"
#include "utils/UuidUtils.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <memory>
#include <optional>
#include <vector>

class AnimationManagerTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<AnimationManager> _animations{nullptr};
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

	std::optional<int> _frame{};
	std::vector<AnimationType> _drawn{};
	std::optional<Uuid> _finished{};

	BulletCalibre _calibre{.speed = 300.0, .damage = 1u, .damageRadius = 12.0, .tier = 1u, .size{.x = 6.0, .y = 5.0}};
	Uuid _uuid{};
	//NOTE: a burst reports only when it has an owner, and a nil uuid is how the manager spells "none"
	Uuid _burstUuid{UuidUtils::GetRandomUuid()};
	ObjRectangle _rect{.x = 0.0, .y = 0.0, .w = 12.0, .h = 12.0};
	unsigned short _health{1};
	int _ticksPerFrame{20};
	//NOTE: a spawn burst is three frames of twenty ticks, so this is well past its last one
	int _ticksPerBurst{120};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_allObjects.reserve(8u);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_animations = std::make_unique<AnimationManager>(_events);

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

		_subs.push_back(_events->AddListener([this](const DrawAnimationEvent& event)
		{
			_frame = event.frame;
			_drawn.push_back(event.type);
		}));
		_subs.push_back(_events->AddListener([this](const SpawnAnimationFinishedEvent& event)
		{
			_finished = event.uuid;
		}));
	}

	void RunTicks(const int count) const
	{
		for (int tick = 0; tick < count; ++tick)
		{
			_events->EmitEvent(PostTickUpdateEvent{.deltaTime = 1.0 / 60.0});
		}
	}

	[[nodiscard]] bool WasDrawn(const AnimationType type) const
	{
		return std::ranges::find(_drawn, type) != _drawn.end();
	}
};

// --- what the game logic asks for, and with which data

TEST_F(AnimationManagerTest, BulletExplodesWhereItHit)
{
	const ObjRectangle bulletRect{.x = 0.0, .y = 0.0, .w = _calibre.size.x, .h = _calibre.size.y};
	auto bullet = TestUtils::CreateBullet(bulletRect, _health, _uuid, _allObjects, _events,
										  _calibre, Direction::DOWN, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);
	_allObjects.emplace_back(std::make_shared<BrickWall>(ObjRectangle{.x = 0.0, .y = 8.0, .w = 12.0, .h = 12.0},
														 _events, _uuid, _gameConfig));

	_events->EmitEvent(TickUpdateEvent{.deltaTime = 1.0 / 60.0});

	ASSERT_TRUE(_bulletExplosion.has_value());
	EXPECT_EQ(_bulletExplosion->rect.y, bullet->GetRect().y);
}

//NOTE: the client never runs the collision itself - the burst has to come from the host's despawn
TEST_F(AnimationManagerTest, ClientBulletExplodesOnDespawn)
{
	_gameConfig.gameMode = GameMode::PlayAsClient;

	const ObjRectangle bulletRect{.x = 20.0, .y = 30.0, .w = _calibre.size.x, .h = _calibre.size.y};
	auto bullet = TestUtils::CreateBullet(bulletRect, _health, _uuid, _allObjects, _events,
										  _calibre, Direction::DOWN, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);

	_events->EmitEvent(Key(bullet->GetUuid()), DespawnedEvent{.uuid = bullet->GetUuid()});

	ASSERT_TRUE(_bulletExplosion.has_value());
	EXPECT_EQ(_bulletExplosion->rect.x, bulletRect.x);
	EXPECT_EQ(_bulletExplosion->rect.y, bulletRect.y);
}

TEST_F(AnimationManagerTest, TankExplodesWhereItDied)
{
	constexpr ObjRectangle tankRect{.x = 40.0, .y = 50.0, .w = 12.0, .h = 12.0};
	auto tank = TestUtils::CreatePlayer(tankRect, _health, _uuid, Author::Player1, _allObjects,
										_events, 1u, Direction::UP, _bulletPool, _gameConfig);

	tank->TakeDamage(static_cast<unsigned int>(tank->GetHealth()), Author::Enemy1);

	ASSERT_TRUE(_tankExplosion.has_value());
	EXPECT_EQ(_tankExplosion->author, Author::Player1);
	EXPECT_EQ(_tankExplosion->rect.x, tankRect.x);
	EXPECT_EQ(_tankExplosion->rect.y, tankRect.y);
}

TEST_F(AnimationManagerTest, ALiveTankTakenOffTheFieldExplodesNothing)
{
	{
		auto tank = TestUtils::CreatePlayer(ObjRectangle{.x = 0.0, .y = 0.0, .w = 12.0, .h = 12.0}, _health, _uuid,
											Author::Player1, _allObjects, _events, 1u,
											Direction::UP, _bulletPool, _gameConfig);
	}

	EXPECT_FALSE(_tankExplosion.has_value());
}

TEST_F(AnimationManagerTest, SpawnedTankAsksForItsSpawnBurst)
{
	std::shared_ptr<RespawnManager> respawnManager{nullptr};
	std::shared_ptr<TankSpawner> tankSpawner{nullptr};
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::OnePlayer, respawnManager, tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});

	ASSERT_TRUE(_tankSpawn.has_value());
	EXPECT_NE(_tankSpawn->uuid, Uuid{});
}

TEST_F(AnimationManagerTest, WaterTileAsksForItsFlowWhenBuilt)
{
	constexpr ObjRectangle waterRect{.x = 24.0, .y = 36.0, .w = 12.0, .h = 12.0};
	const WaterTile water{waterRect, _events, _uuid, _gameConfig};

	ASSERT_TRUE(_water.has_value());
	EXPECT_EQ(_water->rect.x, waterRect.x);
	EXPECT_EQ(_water->rect.y, waterRect.y);
}

TEST_F(AnimationManagerTest, HelmetPickupTurnsTheShieldOnAndOff)
{
	auto tank = TestUtils::CreatePlayer(ObjRectangle{.x = 0.0, .y = 0.0, .w = 12.0, .h = 12.0}, _health, _uuid,
										Author::Player1, _allObjects, _events, 1u,
										Direction::UP, _bulletPool, _gameConfig);

	_events->EmitEvent(Key(Author::Player1), BonusHelmetStatusChangeEvent{.isActive = true});

	ASSERT_TRUE(_helmet.has_value());
	EXPECT_EQ(_helmet->author, Author::Player1);
	EXPECT_TRUE(_helmet->isEnable);

	_events->EmitEvent(Key(Author::Player1), BonusHelmetStatusChangeEvent{.isActive = false});

	EXPECT_FALSE(_helmet->isEnable);
}

// --- the clock: water never ends, so it is always there to be asked its frame

TEST_F(AnimationManagerTest, FrameAdvancesWhileRunning)
{
	_events->EmitEvent(AnimationCreateWaterEvent{.rect = _rect});
	_events->EmitEvent(DrawEvent{});
	ASSERT_TRUE(_frame.has_value());
	const int startFrame = *_frame;

	RunTicks(_ticksPerFrame);

	_events->EmitEvent(DrawEvent{});

	EXPECT_NE(*_frame, startFrame);
}

TEST_F(AnimationManagerTest, FrameStandsStillWhilePaused)
{
	_events->EmitEvent(AnimationCreateWaterEvent{.rect = _rect});
	_events->EmitEvent(DrawEvent{});
	ASSERT_TRUE(_frame.has_value());
	const int startFrame = *_frame;

	_events->EmitEvent(PauseStatusEvent{.isPaused = true});

	RunTicks(_ticksPerFrame * 3);

	_events->EmitEvent(DrawEvent{});

	EXPECT_EQ(*_frame, startFrame);
}

TEST_F(AnimationManagerTest, FrameResumesAfterUnpause)
{
	_events->EmitEvent(AnimationCreateWaterEvent{.rect = _rect});
	_events->EmitEvent(DrawEvent{});
	ASSERT_TRUE(_frame.has_value());
	const int startFrame = *_frame;

	_events->EmitEvent(PauseStatusEvent{.isPaused = true});
	RunTicks(1);
	_events->EmitEvent(PauseStatusEvent{.isPaused = false});

	RunTicks(_ticksPerFrame);

	_events->EmitEvent(DrawEvent{});

	EXPECT_NE(*_frame, startFrame);
}

// --- the spawn burst doubles as the countdown, so its last frame is what lands the tank

TEST_F(AnimationManagerTest, ABurstLeftAloneReportsItsOwner)
{
	_events->EmitEvent(AnimationCreateTankSpawnEvent{.rect = _rect, .uuid = _burstUuid});

	RunTicks(_ticksPerBurst);

	ASSERT_TRUE(_finished.has_value());
	EXPECT_EQ(*_finished, _burstUuid);
}

TEST_F(AnimationManagerTest, ACancelledBurstNeverReports)
{
	_events->EmitEvent(AnimationCreateTankSpawnEvent{.rect = _rect, .uuid = _burstUuid});

	_events->EmitEvent(AnimationCancelTankSpawnEvent{.uuid = _burstUuid});

	RunTicks(_ticksPerBurst);

	EXPECT_FALSE(_finished.has_value());
}

// --- the phase is the layer, and inside one phase the paint order is subscription order

TEST_F(AnimationManagerTest, AnExplosionIsDrawnInThePostDrawPhase)
{
	_events->EmitEvent(AnimationCreateBulletExplosionEvent{.rect = _rect});

	_events->EmitEvent(DrawEvent{});
	EXPECT_FALSE(WasDrawn(AnimationType::Bullet_Explosion));

	_events->EmitEvent(PostDrawEvent{});
	EXPECT_TRUE(WasDrawn(AnimationType::Bullet_Explosion));
}

// Water is terrain, not an overlay - it stays in the main phase, under whatever stands on it
TEST_F(AnimationManagerTest, WaterStaysInTheMainDrawPhase)
{
	_events->EmitEvent(AnimationCreateWaterEvent{.rect = _rect});

	_events->EmitEvent(DrawEvent{});

	EXPECT_TRUE(WasDrawn(AnimationType::Water_Flow));
}
