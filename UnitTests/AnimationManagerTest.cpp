#include "TestUtils.h"
#include "components/ObstacleSpawner.h"
#include "application/GameConfig.h"
#include "application/ProjectConfig.h"
#include "components/AnimatedObjects.h"
#include "components/BulletPool.h"
#include "components/TankPool.h"
#include "components/EventSystem.h"
#include "components/ScenePainter.h"
#include "components/Sprite.h"
#include "components/TankSpawner.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/events/TimingEvents.h"
#include "components/managers/AnimationManager.h"
#include "components/managers/RespawnManager.h"
#include "components/managers/TextureManager.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Tank.h"
#include "enums/AnimationType.h"
#include "enums/Direction.h"
#include "enums/DrawLayer.h"
#include "enums/GameMode.h"
#include "enums/TextureOffset.h"
#include "enums/TextureType.h"
#include "utils/MathUtils.h"
#include "utils/Uuid.h"
#include "utils/UuidUtils.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <memory>
#include <optional>
#include <string_view>
#include <vector>

// what the manager is asked to play and when: an event goes in, frames are advanced by hand, bursts are read
class AnimationManagerTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::unique_ptr<AnimationManager> _animations{nullptr};
	ProjectConfig _projectConfig{"", true};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<TankPool> _tankPool{nullptr};
	std::vector<EventSubscription> _subs{};
	EventSubscription _spawnQueueSub{};

	std::optional<AnimationCreateBulletExplosionEvent> _bulletExplosion{};
	std::optional<AnimationCreateTankExplosionEvent> _tankExplosion{};
	std::optional<AnimationCreateTankSpawnEvent> _tankSpawn{};
	std::optional<AnimationCreateWaterEvent> _water{};

	std::optional<Uuid> _finished{};

	BulletCaliber _caliber{.speed = 300.0, .damage = 1u, .damageRadius = 12.0, .tier = 1u, .size{.x = 6.0, .y = 5.0}};
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
		_obstacleSpawner = std::make_unique<ObstacleSpawner>(_events, _gameConfig);
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_allObjects.reserve(8u);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_tankPool = std::make_shared<TankPool>(_events, _allObjects, _gameConfig, _bulletPool);
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

	//NOTE: a disposed burst keeps its slot, but nobody paints it
	[[nodiscard]] bool IsPlaying(const AnimationType type) const
	{
		return std::ranges::any_of(_animations->OnLayer(DrawLayer::Effects), [type](const AnimatedObject& animation)
		{
			return animation.type == type && !animation.markToDispose;
		});
	}

	std::shared_ptr<Tank> CreatePlayer(const FPoint pos)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};
		auto player{TestUtils::CreatePlayer(rect, _health, Author::Player1, _allObjects, _events, Direction::UP,
											_tankPool, _gameConfig)};

		return player;
	}

	std::shared_ptr<Bullet> CreateBullet(const FPoint pos)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _caliber.size.x, .h = _caliber.size.y};
		auto bullet{TestUtils::CreateBullet(rect, _health, _bulletPool, _events, _caliber, Direction::DOWN,
											Author::Player1)};

		return bullet;
	}

	std::shared_ptr<BaseObj> SpawnObstacle(const ObjRectangle rect, const ObstacleType type) const
	{
		return TestUtils::SpawnObstacle(_events, _allObjects, rect, type);
	}
};

// a bullet dying on a target leaves its blast at the point of impact
TEST_F(AnimationManagerTest, BulletExplodesWhereItHit)
{
	const auto bullet{CreateBullet({.x = 0.0, .y = 0.0})};
	_allObjects.emplace_back(SpawnObstacle(ObjRectangle{.x = 0.0, .y = 8.0, .w = 12.0, .h = 12.0},
										   ObstacleType::Brick));

	_events->EmitEvent(TickUpdateEvent{.deltaTime = 1.0 / 60.0});

	ASSERT_TRUE(_bulletExplosion.has_value());
	EXPECT_EQ(_bulletExplosion->rect.y, bullet->GetRect().y);
}

//NOTE: the client never runs the collision itself - the burst has to come from the server's despawn
TEST_F(AnimationManagerTest, ClientBulletExplodesOnDespawn)
{
	_gameConfig.gameMode = GameMode::PlayAsClient;

	const auto bullet{CreateBullet({.x = 20.0, .y = 30.0})};

	_events->EmitEvent(Key(bullet->GetUuid()), DespawnedEvent{.uuid = bullet->GetUuid()});

	ASSERT_TRUE(_bulletExplosion.has_value());
	EXPECT_EQ(_bulletExplosion->rect.x, bullet->GetRect().x);
	EXPECT_EQ(_bulletExplosion->rect.y, bullet->GetRect().y);
}

// a tank that died explodes where it stood
TEST_F(AnimationManagerTest, TankExplodesWhereItDied)
{
	const auto tank{CreatePlayer({.x = 40.0, .y = 50.0})};

	tank->TakeDamage(static_cast<unsigned int>(tank->GetHealth()), Author::Enemy1);

	ASSERT_TRUE(_tankExplosion.has_value());
	EXPECT_EQ(_tankExplosion->rect.x, tank->GetRect().x);
	EXPECT_EQ(_tankExplosion->rect.y, tank->GetRect().y);
}

// one taken off the field alive does not - a reset is not a death
TEST_F(AnimationManagerTest, ALiveTankTakenOffTheFieldExplodesNothing)
{
	{
		auto tank{CreatePlayer({.x = 0.0, .y = 0.0})};
	}

	EXPECT_FALSE(_tankExplosion.has_value());
}

// a spawned tank asks for its own flash
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

// and a water tile for its flow as soon as it is built
TEST_F(AnimationManagerTest, WaterTileAsksForItsFlowWhenBuilt)
{
	constexpr ObjRectangle waterRect{.x = 24.0, .y = 36.0, .w = 12.0, .h = 12.0};
	const WaterTile water{waterRect, _events, _uuid, _gameConfig};

	ASSERT_TRUE(_water.has_value());
	EXPECT_EQ(_water->rect.x, waterRect.x);
	EXPECT_EQ(_water->rect.y, waterRect.y);
}

// the shield follows the effect: on with the pickup, off when it runs out
TEST_F(AnimationManagerTest, HelmetPickupTurnsTheShieldOnAndOff)
{
	auto tank{CreatePlayer({.x = 0.0, .y = 0.0})};

	_events->EmitEvent(Key(Author::Player1), BonusHelmetStatusChangeEvent{.isActive = true});

	const std::optional<Sprite> shield{tank->Cover()};
	ASSERT_TRUE(shield.has_value());
	EXPECT_EQ(shield->texture, TextureType::Helmet);
	EXPECT_EQ(shield->rect.x, tank->GetRect().x);

	_events->EmitEvent(Key(Author::Player1), BonusHelmetStatusChangeEvent{.isActive = false});

	EXPECT_FALSE(tank->Cover().has_value());
}

// --- the clock: water never ends, so it is always there to be asked its frame

// frames move on with the clock while the match runs
TEST_F(AnimationManagerTest, FrameAdvancesWhileRunning)
{
	_events->EmitEvent(AnimationCreateWaterEvent{.rect = _rect});
	ASSERT_EQ(_animations->OnLayer(DrawLayer::Ground).size(), 1u);
	const int startFrame{_animations->OnLayer(DrawLayer::Ground).front().currentFrameIndex};

	RunTicks(_ticksPerFrame);

	EXPECT_NE(_animations->OnLayer(DrawLayer::Ground).front().currentFrameIndex, startFrame);
}

// stand still under a pause
TEST_F(AnimationManagerTest, FrameStandsStillWhilePaused)
{
	_events->EmitEvent(AnimationCreateWaterEvent{.rect = _rect});
	ASSERT_EQ(_animations->OnLayer(DrawLayer::Ground).size(), 1u);
	const int startFrame{_animations->OnLayer(DrawLayer::Ground).front().currentFrameIndex};

	_events->EmitEvent(PauseStatusEvent{.isPaused = true});

	RunTicks(_ticksPerFrame * 3);

	EXPECT_EQ(_animations->OnLayer(DrawLayer::Ground).front().currentFrameIndex, startFrame);
}

// and pick up again afterwards, rather than catching up on what was missed
TEST_F(AnimationManagerTest, FrameResumesAfterUnpause)
{
	_events->EmitEvent(AnimationCreateWaterEvent{.rect = _rect});
	ASSERT_EQ(_animations->OnLayer(DrawLayer::Ground).size(), 1u);
	const int startFrame{_animations->OnLayer(DrawLayer::Ground).front().currentFrameIndex};

	_events->EmitEvent(PauseStatusEvent{.isPaused = true});
	RunTicks(1);
	_events->EmitEvent(PauseStatusEvent{.isPaused = false});

	RunTicks(_ticksPerFrame);

	EXPECT_NE(_animations->OnLayer(DrawLayer::Ground).front().currentFrameIndex, startFrame);
}

// --- the spawn burst doubles as the countdown, so its last frame is what lands the tank

// a burst that plays to the end names the object it belonged to
TEST_F(AnimationManagerTest, ABurstLeftAloneReportsItsOwner)
{
	_events->EmitEvent(AnimationCreateTankSpawnEvent{.rect = _rect, .uuid = _burstUuid});

	RunTicks(_ticksPerBurst);

	ASSERT_TRUE(_finished.has_value());
	EXPECT_EQ(*_finished, _burstUuid);
}

// a cancelled one says nothing at all
TEST_F(AnimationManagerTest, ACancelledBurstNeverReports)
{
	_events->EmitEvent(AnimationCreateTankSpawnEvent{.rect = _rect, .uuid = _burstUuid});

	_events->EmitEvent(AnimationCancelTankSpawnEvent{.uuid = _burstUuid});

	RunTicks(_ticksPerBurst);

	EXPECT_FALSE(_finished.has_value());
}

// and the cancel takes only that burst, not everything the owner has running
TEST_F(AnimationManagerTest, ACancelLeavesTheOwnersOtherAnimations)
{
	_events->EmitEvent(AnimationCreateBonusSpawnEvent{.rect = _rect, .uuid = _burstUuid});

	_events->EmitEvent(AnimationCancelTankSpawnEvent{.uuid = _burstUuid});

	RunTicks(_ticksPerBurst * 2);

	EXPECT_TRUE(_finished.has_value());
}

//NOTE: a client's burst waits for the server's word, so it has to outlast a burst of its own
TEST_F(AnimationManagerTest, AnEndlessTankBurstPlaysUntilTheTankLands)
{
	_events->EmitEvent(AnimationCreateTankSpawnEvent{.rect = _rect, .uuid = _burstUuid, .isEndless = true});

	RunTicks(_ticksPerBurst);

	EXPECT_FALSE(_finished.has_value());
	EXPECT_TRUE(IsPlaying(AnimationType::Tank_Spawn));

	_events->EmitEvent(TankSpawnCompletedEvent{.uuid = _burstUuid});

	EXPECT_FALSE(IsPlaying(AnimationType::Tank_Spawn));
}

// the same for a bonus
TEST_F(AnimationManagerTest, AnEndlessBonusBurstPlaysUntilTheBonusLands)
{
	_events->EmitEvent(AnimationCreateBonusSpawnEvent{.rect = _rect, .uuid = _burstUuid, .isEndless = true});

	RunTicks(_ticksPerBurst * 2);

	EXPECT_FALSE(_finished.has_value());
	EXPECT_TRUE(IsPlaying(AnimationType::Bonus_Spawn));

	_events->EmitEvent(BonusSpawnCompletedEvent{.uuid = _burstUuid});

	EXPECT_FALSE(IsPlaying(AnimationType::Bonus_Spawn));
}

//NOTE: a finished burst's slot is reused - a client that watched the demo first must not inherit its end
TEST_F(AnimationManagerTest, AnEndlessBurstInAReusedSlotStaysEndless)
{
	_events->EmitEvent(AnimationCreateTankSpawnEvent{.rect = _rect, .uuid = _burstUuid});
	RunTicks(_ticksPerBurst);
	ASSERT_TRUE(_finished.has_value());

	_finished.reset();
	_events->EmitEvent(AnimationCreateTankSpawnEvent{.rect = _rect, .uuid = _burstUuid, .isEndless = true});
	RunTicks(_ticksPerBurst);

	EXPECT_FALSE(_finished.has_value());
}

// --- the painter paints layer by layer, whatever order the parts were built in

// water under the wall standing beside it, a blast over the wall, the bush over the blast - cover is meant to hide
TEST_F(AnimationManagerTest, TheLayersArePaintedGroundWorldEffectsCover)
{
	const auto isAt = [](const RenderTextureEvent& event, const ObjRectangle& atlas)
	{
		return MathUtils::AreEqualAbsolute(event.textureRect.x, atlas.x)
			   && MathUtils::AreEqualAbsolute(event.textureRect.y, atlas.y);
	};
	std::vector<std::string_view> painted{};
	const EventSubscription paintSub{_events->AddListener([&painted, &isAt](const RenderTextureEvent& event)
	{
		if (isAt(event, TextureOffset::kWater))
		{
			painted.emplace_back("water");
		}
		else if (isAt(event, TextureOffset::kBrick))
		{
			painted.emplace_back("wall");
		}
		else if (isAt(event, TextureOffset::kBulletExplosion))
		{
			painted.emplace_back("blast");
		}
		else if (isAt(event, TextureOffset::kBush))
		{
			painted.emplace_back("bush");
		}
	})};
	const TextureManager textures{_events};
	ScenePainter painter{_events, _allObjects, *_animations, textures};

	SpawnObstacle(_rect, ObstacleType::Bush);
	_events->EmitEvent(AnimationCreateBulletExplosionEvent{.rect = _rect});
	SpawnObstacle(_rect, ObstacleType::Brick);
	_events->EmitEvent(AnimationCreateWaterEvent{.rect = _rect});

	painter.Paint();

	EXPECT_EQ(painted, (std::vector<std::string_view>{"water", "wall", "blast", "bush"}));
}
