#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/TankPool.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/TimingEvents.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "components/managers/BonusManager.h"
#include "entities/bonuses/Bonus.h"
#include "components/ObstacleSpawner.h"
#include "components/managers/FortressManager.h"
#include "entities/obstacles/FortressWalls.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/BulletResetProperty.h"
#include "entities/pawns/Tank.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/Faction.h"
#include "enums/GameMode.h"
#include "enums/ObstacleType.h"
#include "enums/InputChannel.h"
#include "utils/UuidUtils.h"
#include "geometry/Point.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <array>
#include <chrono>
#include <cstddef>
#include <memory>
#include <optional>
#include <string_view>
#include <thread>
#include <vector>

using namespace std::chrono_literals;

// each case is a pair: a bonus dropped a step in front of a tank, and the same bonus left out of reach
class BonusTest : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<TankPool> _tankPool{nullptr};
	std::unique_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::vector<EventSubscription> _instantSpawnAnimationSubs{};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<RespawnManager> _respawnManager{nullptr};
	std::shared_ptr<BonusManager> _bonusManager{nullptr};
	std::unique_ptr<FortressManager> _fortressManager{nullptr};
	std::unique_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::shared_ptr<BaseObj> _fortressWall{nullptr};
	EventSubscription _fortressWallSub{};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	unsigned short _tankHealth{100u};
	unsigned short _bulletHealth{1u};
	double _tankSize{};
	double _gridSize{};
	double _deltaTimeOneFrame{1.0 / 60.0};
	BulletCaliber _caliber{.speed = 300.0, .damage = 1u, .damageRadius = 12.0, .tier = 1u, .size{.x = 6.0, .y = 5.0}};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_tankPool = std::make_shared<TankPool>(_events, _allObjects, _gameConfig, _bulletPool);
		TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, _gameConfig.gameMode, _respawnManager,
								 _tankSpawner);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, _allObjects, _gameConfig);
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
		_bonusManager = std::make_unique<BonusManager>(_events, _gameConfig);
		_fortressManager = std::make_unique<FortressManager>(_events, _allObjects, _gameConfig);
		_obstacleSpawner = std::make_unique<ObstacleSpawner>(_events, _gameConfig);
		_fortressWallSub = TestUtils::TrackFortressWall(_events, &_fortressWall);
		_gridSize = _gameConfig.gridOffset;
		_tankSize = _gameConfig.tankSize;

		_allObjects.reserve(4);
	}

	void TearDown() override {}

	std::shared_ptr<Tank> CreatePlayer(const FPoint pos, const Author author = Author::Player1,
									   const Direction dir = Direction::UP, const unsigned short tier = 1u)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};
		auto player{TestUtils::CreatePlayer(rect, _tankHealth, author, _allObjects, _events, dir, _tankPool,
											_gameConfig, tier)};

		return player;
	}

	std::shared_ptr<Tank> CreateBot(const FPoint pos, const Author author, const Direction dir,
									const unsigned short tier = 1u)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};
		auto bot{TestUtils::CreateBot(rect, _tankHealth, author, _allObjects, _events, dir, _tankPool, _gameConfig,
									  tier)};

		return bot;
	}

	std::shared_ptr<Bullet> CreateBullet(const FPoint pos, const Direction dir, const Author author)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _caliber.size.x, .h = _caliber.size.y};
		auto bullet{TestUtils::CreateBullet(rect, _bulletHealth, _bulletPool, _events, _caliber, dir,
											author)};

		return bullet;
	}
};

// a bonus lying untouched stays; driving onto it takes it off the field
TEST_F(BonusTest, BonusPickUp)
{
	CreatePlayer({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN);
	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize});
	const auto* const bonus{dynamic_cast<Bonus*>(_allObjects.back().get())};
	ASSERT_NE(bonus, nullptr);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	ASSERT_TRUE(bonus->GetIsAlive()) << "the bonus went before anyone reached it";

	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = true});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(bonus->GetIsAlive());
}

// the timer freezes the enemy team where it stands - a team that was driving while it lay untouched
TEST_F(BonusTest, TimerPickUpEnemyCantMove)
{
	CreatePlayer({.x = 0.0, .y = 0.0});
	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Timer);

	const auto enemyBot{CreateBot({.x = _tankSize * 2, .y = _tankSize * 2}, Author::Enemy1, Direction::DOWN)};
	const FPoint startPos{enemyBot->GetPos()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	ASSERT_NE(startPos, enemyBot->GetPos()) << "the enemy stood still with the timer still lying there";

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const FPoint enemyPos{enemyBot->GetPos()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(enemyPos, enemyBot->GetPos());
}

// the timer freezes the whole team: an enemy arriving during it arrives frozen, without one it drives off
TEST_F(BonusTest, AnEnemyArrivingDuringAFreezeArrivesFrozen)
{
	constexpr std::array kWithAndWithoutTimer{false, true};
	for (const bool isTimerTaken: kWithAndWithoutTimer)
	{
		SCOPED_TRACE(isTimerTaken);
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		if (isTimerTaken)
		{
			_bonusSpawner->SpawnBonus({.w = _tankSize, .h = _tankSize}, BonusType::Timer);
			const auto timer{std::dynamic_pointer_cast<Bonus>(_allObjects.back())};
			ASSERT_NE(timer, nullptr);
			timer->PickUpBonus(Author::Player1);
		}

		_events->EmitEvent(RespawnTanksEvent{});
		const auto enemy{std::ranges::find(_allObjects, Faction::EnemyTeam, &BaseObj::GetFaction)};
		ASSERT_NE(enemy, _allObjects.end()) << "no enemy came up";
		const std::shared_ptr<BaseObj> arrived{*enemy};
		const FPoint arrivedAt{arrived->GetPos()};

		for (int frame{}; frame < 10; ++frame)
		{
			_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		}

		EXPECT_EQ(arrived->GetPos() == arrivedAt, isTimerTaken);
	}
}

// a second timer adds its time: the enemy still stands past the first one's time and drives once both ran out
TEST_F(BonusTest, AnotherPlayersTimerExtendsTheFreeze)
{
	_gameConfig.bonusEffectDuration = 150ms;
	const auto enemyBot{CreateBot({.x = _tankSize * 2.0, .y = _tankSize * 2.0}, Author::Enemy1, Direction::DOWN)};
	constexpr std::array kTakers{Author::Player1, Author::Player2};
	for (const Author taker: kTakers)
	{
		_bonusSpawner->SpawnBonus({.w = _tankSize, .h = _tankSize}, BonusType::Timer);
		const auto timer{std::dynamic_pointer_cast<Bonus>(_allObjects.back())};
		ASSERT_NE(timer, nullptr);
		timer->PickUpBonus(taker);
	}

	std::this_thread::sleep_for(225ms);
	const FPoint frozenAt{enemyBot->GetPos()};
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	ASSERT_EQ(frozenAt, enemyBot->GetPos()) << "the freeze ended with the first timer's time";

	std::this_thread::sleep_for(125ms);
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_NE(frozenAt, enemyBot->GetPos()) << "the freeze outlived both timers";
}

// the helmet turns away the shot that wounds the tank without it
TEST_F(BonusTest, HelmetPickUpAndBulletCantDamageTank)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};
	const unsigned int spawnHealth{player->GetHealth()};
	CreateBullet({.x = _tankSize + 1.0, .y = 0.0}, Direction::LEFT, Author::Enemy1);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	ASSERT_GT(spawnHealth, player->GetHealth()) << "the shot missed the tank even without a helmet";

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});
	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Helmet);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	//NOTE: taken after the pickup - the pickup heals
	const unsigned int playerHealth{player->GetHealth()};
	CreateBullet({.x = _tankSize + 1.0, .y = 0.0}, Direction::LEFT, Author::Enemy1);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(playerHealth, player->GetHealth());
}

// lying untouched the grenade does nothing; taken, it empties the health of every enemy on the field
TEST_F(BonusTest, GrenadePickUpEnemyHealthZero)
{
	CreatePlayer({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN);
	const auto enemyBot{CreateBot({.x = _tankSize * 2, .y = _tankSize * 2}, Author::Enemy1, Direction::DOWN)};
	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Grenade);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	ASSERT_EQ(enemyBot->GetHealth(), 100u) << "the grenade went off before anyone reached it";

	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = true});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(enemyBot->GetHealth(), 0u);
}

// the tank bonus is a life once taken, not while it lies there: the respawn count of the seat grows
TEST_F(BonusTest, TankPickUpExtraLife)
{
	unsigned short respawnActual{3u};
	auto respawnSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		respawnActual = event.respawnCount;
	})};

	CreatePlayer({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN);
	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Tank);
	const unsigned short playerSpawnCount{respawnActual};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	ASSERT_EQ(playerSpawnCount, respawnActual) << "the life was given before anyone reached the bonus";

	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = true});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(playerSpawnCount, respawnActual);
}

// a star is one tier, a super star two, the caliber three - and only once the tank has driven onto it
TEST_F(BonusTest, AStarOrCaliberRaisesTheTier)
{
	struct Pickup final
	{
		std::string_view why;
		BonusType bonus;
		bool isSuper{};
		unsigned short tiers{};
	};

	constexpr std::array kPickups{Pickup{.why = "a star", .bonus = BonusType::Star, .tiers = 1u},
								  Pickup{.why = "a super star", .bonus = BonusType::Star, .isSuper = true, .tiers = 2u},
								  Pickup{.why = "the caliber", .bonus = BonusType::Caliber, .tiers = 3u}};
	for (const Pickup& pickup: kPickups)
	{
		SCOPED_TRACE(pickup.why);
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		const auto player{CreatePlayer({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};
		_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, pickup.bonus, {},
								  pickup.isSuper);

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		ASSERT_EQ(player->GetTier(), 1u) << "the bonus was taken before the tank reached it";

		_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = true});
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(player->GetTier(), 1u + pickup.tiers);
	}
}

// any bonus heals the tank that took it, past its spawn health - a star at the top tier too, with no tier to give
TEST_F(BonusTest, APickupHealsTheTank)
{
	struct Pickup final
	{
		BonusType bonus;
		unsigned short tier{};
	};

	//NOTE: a maxed tank returns early from the star handler - the heal must not hang on the tier going up
	constexpr std::array kPickups{Pickup{.bonus = BonusType::Helmet, .tier = 1u},
								  Pickup{.bonus = BonusType::Star, .tier = 7u}};
	for (const Pickup& pickup: kPickups)
	{
		SCOPED_TRACE(static_cast<int>(pickup.bonus));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		const auto player{CreatePlayer({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN, pickup.tier)};
		const unsigned int spawnHealth{player->GetHealth()};
		_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = true});
		_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, pickup.bonus);

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_GT(player->GetHealth(), spawnHealth);
	}
}

// the shovel turns the eagle's brick wall into steel once taken, not while it lies there
TEST_F(BonusTest, ShovelPickUpByPlayerThenFortressWallTurnIntoSteelWall)
{
	CreatePlayer({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN);
	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Shovel);
	const ObjRectangle fortressRect{.x = _tankSize + 1.0, .y = 0, .w = _gridSize, .h = _gridSize};
	_events->EmitEvent(SpawnObstacleEvent{.rect = fortressRect, .type = ObstacleType::Fortress});

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	ASSERT_NE(dynamic_cast<FortressBrickWall*>(_fortressWall.get()), nullptr) << "the walls turned before the pickup";

	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = true});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_NE(dynamic_cast<FortressSteelWall*>(_fortressWall.get()), nullptr);
}

// and heals the eagle at once
TEST_F(BonusTest, ShovelPickUpByPlayerHealsTheEagle)
{
	_events->EmitEvent(SpawnObstacleEvent{.rect = {.x = 0.0, .y = 0.0, .w = _gridSize, .h = _gridSize},
										  .type = ObstacleType::Eagle});
	const std::shared_ptr<BaseObj> eagle{_allObjects.back()};
	const unsigned int fullHealth{eagle->GetHealth()};
	eagle->TakeDamage(1u, Author::Enemy1);

	_events->EmitEvent(BonusShovelPickupEvent{.faction = Faction::PlayerTeam});

	EXPECT_EQ(fullHealth, eagle->GetHealth());
}

// but not one shot down in the same tick
TEST_F(BonusTest, ShovelDoesNotHealAFallenEagle)
{
	_events->EmitEvent(SpawnObstacleEvent{.rect = {.x = 0.0, .y = 0.0, .w = _gridSize, .h = _gridSize},
										  .type = ObstacleType::Eagle});
	const std::shared_ptr<BaseObj> eagle{_allObjects.back()};
	eagle->TakeDamage(eagle->GetHealth(), Author::Enemy1);

	_events->EmitEvent(BonusShovelPickupEvent{.faction = Faction::PlayerTeam});

	EXPECT_FALSE(eagle->GetIsAlive());
	EXPECT_EQ(0u, eagle->GetHealth());
}

// and lets the same tank across once the ship is taken
TEST_F(BonusTest, ShipPickUpCanCrossWater)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0})};
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Ship);
	const ObjRectangle waterRect{.x = 0.0, .y = _tankSize * 2.0 + 2.0, .w = _gridSize, .h = _gridSize};
	_events->EmitEvent(SpawnObstacleEvent{.rect = waterRect, .type = ObstacleType::Water});

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	//NOTE: no Game here to sweep the dead bonus out of the way
	std::erase_if(_allObjects, [](const std::shared_ptr<BaseObj>& object) { return !object->GetIsAlive(); });

	constexpr int framesToCrossWater{100};
	for (int frame = 0; frame < framesToCrossWater; ++frame)
	{
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	}

	EXPECT_GT(player->GetRect().y, waterRect.y + waterRect.h);
}

// a dropped bonus waits out its burst: the world stays empty until the animation reports the end
TEST_F(BonusTest, DelayedSpawnLandsAfterAnimation)
{
	std::optional<AnimationCreateBonusSpawnEvent> burst{};
	auto animationSub{_events->AddListener(
			[&burst](const AnimationCreateBonusSpawnEvent& event) { burst = event; })};

	//NOTE: this one is about the wait itself, so it drops the fixture's stand-in for the animation
	_instantSpawnAnimationSubs.clear();

	const Uuid uuid{UuidUtils::GetRandomUuid()};
	_bonusSpawner->SpawnBonus({.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize}, BonusType::Star, uuid);

	ASSERT_TRUE(burst.has_value());
	EXPECT_FALSE(burst->isEndless);
	EXPECT_TRUE(_allObjects.empty());

	_events->EmitEvent(SpawnAnimationFinishedEvent{.uuid = uuid});

	ASSERT_EQ(_allObjects.size(), 1u);
	EXPECT_NE(dynamic_cast<Bonus*>(_allObjects.back().get()), nullptr);
}

// a map-laid bonus keeps no clock: with lifetime zero a dropped one is swept next tick, the placed one stays
TEST_F(BonusTest, ABonusLaidOutByTheMapNeverExpires)
{
	_gameConfig.bonusLifeTimeCooldown = std::chrono::milliseconds{};

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize}, BonusType::Helmet);
	const auto dropped{_allObjects.back()};

	_events->EmitEvent(SpawnMapBonusEvent{.rect = {.x = _tankSize * 2.0, .y = 0.0, .w = _tankSize, .h = _tankSize},
										  .type = BonusType::Star});
	const auto placed{_allObjects.back()};
	ASSERT_NE(dropped, placed);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(dropped->GetIsAlive()) << "the dropped one outlived its cooldown";
	EXPECT_TRUE(placed->GetIsAlive());
}

// a bonus lands neither in a wall nor in a bush - nobody would see it there
TEST_F(BonusTest, ABonusNeverLandsUnderCover)
{
	_gameConfig.bonusSpawnCooldown = 0ms;
	const auto spot{static_cast<std::size_t>(_gameConfig.bonusSize)};
	_gameConfig.battlefieldSize = UPoint{.x = spot, .y = spot};
	_events->EmitEvent(WorldGeometryChangedEvent{});

	constexpr std::array kCovers{ObstacleType::Bush, ObstacleType::Brick};
	for (const ObstacleType cover: kCovers)
	{
		SCOPED_TRACE(cover == ObstacleType::Bush ? "bush" : "brick");
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		_events->EmitEvent(SpawnObstacleEvent{.rect = {.w = _gridSize, .h = _gridSize}, .type = cover});

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(_allObjects.size(), 1u);
	}
}

// a free-for-all grenade spares only its taker
TEST_F(BonusTest, AFreeForAllGrenadeTakesEveryoneButItsTaker)
{
	_gameConfig.gameMode = GameMode::FreeForAll;
	const auto taker{CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN)};
	const auto otherBot{CreateBot({.x = _tankSize * 2.0, .y = 0.0}, Author::Enemy2, Direction::DOWN)};
	const auto player{CreatePlayer({.x = _tankSize * 4.0, .y = 0.0})};
	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize}, BonusType::Grenade);
	const auto grenade{std::dynamic_pointer_cast<Bonus>(_allObjects.back())};
	ASSERT_NE(grenade, nullptr);

	grenade->PickUpBonus(Author::Enemy1);

	EXPECT_GT(taker->GetHealth(), 0u);
	EXPECT_EQ(otherBot->GetHealth(), 0u);
	EXPECT_EQ(player->GetHealth(), 0u);
}

// and a free-for-all timer freezes everyone but its taker
TEST_F(BonusTest, AFreeForAllTimerFreezesEveryoneButItsTaker)
{
	_gameConfig.gameMode = GameMode::FreeForAll;
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0})};
	const auto taker{CreateBot({.x = _tankSize * 2.0, .y = _tankSize * 2.0}, Author::Enemy1, Direction::DOWN)};
	_bonusSpawner->SpawnBonus({.x = _tankSize * 6.0, .y = _tankSize * 6.0, .w = _tankSize, .h = _tankSize},
							  BonusType::Timer);
	const auto timer{std::dynamic_pointer_cast<Bonus>(_allObjects.back())};
	ASSERT_NE(timer, nullptr);

	timer->PickUpBonus(Author::Enemy1);
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveRightEvent{.isPressed = true});
	const FPoint playerPos{player->GetPos()};
	const FPoint takerPos{taker->GetPos()};
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(playerPos, player->GetPos()) << "the player drove on under another's timer";
	EXPECT_NE(takerPos, taker->GetPos()) << "the timer froze the one who took it";
}

// a shot bonus leaves the field
TEST_F(BonusTest, BonusDestroy)
{
	CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Enemy1);

	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = 7.0, .w = _tankSize, .h = _tankSize});

	const auto* const bonus{dynamic_cast<Bonus*>(_allObjects.back().get())};
	ASSERT_NE(bonus, nullptr);
	EXPECT_TRUE(bonus->GetIsAlive());

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(bonus->GetIsAlive());
}

// and gives its effect to nobody, the shooter on the field included - every type is despawned alike, a star for all
TEST_F(BonusTest, AShotBonusGivesTheShooterNothing)
{
	const auto shooter{CreatePlayer({.x = _tankSize * 3.0, .y = 0.0}, Author::Player1)};
	CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = 7.0, .w = _tankSize, .h = _tankSize}, BonusType::Star);
	const auto* const bonus{dynamic_cast<Bonus*>(_allObjects.back().get())};
	ASSERT_NE(bonus, nullptr);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	ASSERT_FALSE(bonus->GetIsAlive()) << "the shot missed the bonus";
	EXPECT_EQ(shooter->GetTier(), 1u);
}

// the wall is brick when the enemy takes the shovel
TEST_F(BonusTest, ShovelPickUpByEnemyThenFortressBrickWallHide)
{
	const auto enemyBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN)};

	const ObjRectangle fortressRect{.x = _tankSize + 1.0, .y = 0, .w = _gridSize, .h = _gridSize};
	_events->EmitEvent(SpawnObstacleEvent{.rect = fortressRect, .type = ObstacleType::Fortress});

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Shovel);

	EXPECT_NE(dynamic_cast<FortressBrickWall*>(_fortressWall.get()), nullptr);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(_fortressWall->GetIsAlive());
}

// a player takes a shovel first, so the enemy's finds steel - it goes just the same
TEST_F(BonusTest, ShovelPickUpByEnemyThenFortressSteelWallHide)
{
	const auto enemyBot{CreateBot({.x = 0.0, .y = 0.0}, Author::Enemy1, Direction::DOWN)};

	_allObjects.reserve(4);
	CreatePlayer({.x = _tankSize * 2.0, .y = _tankSize * 2.0}, Author::Player1, Direction::DOWN);
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});

	const ObjRectangle fortressRect{.x = _tankSize * 3.0 + 1.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize};
	_events->EmitEvent(SpawnObstacleEvent{.rect = fortressRect, .type = ObstacleType::Fortress});

	const ObjRectangle enemyBonusRect{.x = 0.0, .y = _tankSize + 3.0, .w = _tankSize, .h = _tankSize};

	_bonusSpawner->SpawnBonus(enemyBonusRect, BonusType::Shovel);
	const ObjRectangle playerBonusRect{.x = _tankSize * 2.0,
									   .y = _tankSize * 2.0 + _tankSize + 1.0,
									   .w = _tankSize,
									   .h = _tankSize};
	_bonusSpawner->SpawnBonus(playerBonusRect, BonusType::Shovel);

	EXPECT_NE(dynamic_cast<FortressBrickWall*>(_fortressWall.get()), nullptr);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_NE(dynamic_cast<FortressSteelWall*>(_fortressWall.get()), nullptr);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(_fortressWall->GetIsAlive());
}

// the enemy's shovel takes the walls away and leaves the eagle's health alone
TEST_F(BonusTest, ShovelPickUpByEnemyLeavesTheEagleWounded)
{
	_events->EmitEvent(SpawnObstacleEvent{.rect = {.x = 0.0, .y = 0.0, .w = _gridSize, .h = _gridSize},
										  .type = ObstacleType::Eagle});
	const std::shared_ptr<BaseObj> eagle{_allObjects.back()};
	eagle->TakeDamage(1u, Author::Player1);
	const unsigned int woundedHealth{eagle->GetHealth()};

	_events->EmitEvent(BonusShovelPickupEvent{.faction = Faction::EnemyTeam});

	EXPECT_EQ(woundedHealth, eagle->GetHealth());
}

// the enemy shovel ends the player's effect - left running, it swallows the next pickup as an extension
TEST_F(BonusTest, PlayerShovelWorksAgainAfterAnEnemyShovel)
{
	const ObjRectangle fortressRect{.x = _tankSize * 3.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize};
	_events->EmitEvent(SpawnObstacleEvent{.rect = fortressRect, .type = ObstacleType::Fortress});
	ASSERT_NE(dynamic_cast<FortressBrickWall*>(_fortressWall.get()), nullptr);

	_events->EmitEvent(BonusShovelPickupEvent{.faction = Faction::PlayerTeam});
	ASSERT_NE(dynamic_cast<FortressSteelWall*>(_fortressWall.get()), nullptr);

	_events->EmitEvent(BonusShovelPickupEvent{.faction = Faction::EnemyTeam});
	ASSERT_FALSE(_fortressWall->GetIsAlive());

	_events->EmitEvent(BonusShovelPickupEvent{.faction = Faction::PlayerTeam});

	EXPECT_NE(dynamic_cast<FortressSteelWall*>(_fortressWall.get()), nullptr);
	EXPECT_TRUE(_fortressWall->GetIsAlive());
}
