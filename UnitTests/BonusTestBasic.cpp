#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/TankPool.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/BonusPickupEvents.h"
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
#include <chrono>
#include <memory>
#include <optional>

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
									   const Direction dir = Direction::UP)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};
		auto player{TestUtils::CreatePlayer(rect, _tankHealth, author, _allObjects, _events, dir, _tankPool,
											_gameConfig)};

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

// driving onto the bonus takes it off the field
TEST_F(BonusTest, BonusPickUp)
{
	CreatePlayer({.x = 0.0, .y = 0.0});
	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize});

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});

	if (const auto bonus{dynamic_cast<Bonus*>(_allObjects.back().get())})
	{
		EXPECT_TRUE(bonus->GetIsAlive());

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_FALSE(bonus->GetIsAlive());
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

// driving away from it leaves it lying there
TEST_F(BonusTest, BonusNotPickUp)
{
	CreatePlayer({.x = 0.0, .y = 0.0});

	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize});

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = isPressed});

	if (const auto bonus{dynamic_cast<Bonus*>(_allObjects.back().get())})
	{
		EXPECT_TRUE(bonus->GetIsAlive());

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_TRUE(bonus->GetIsAlive());
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

// the timer freezes the enemy team where it stands
TEST_F(BonusTest, TimerPickUpEnemyCantMove)
{
	CreatePlayer({.x = 0.0, .y = 0.0});
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Timer);

	const auto enemyBot{CreateBot({.x = _tankSize * 2, .y = _tankSize * 2}, Author::Enemy1, Direction::DOWN)};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const FPoint enemyPos{enemyBot->GetPos()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(enemyPos, enemyBot->GetPos());
}

// a second player's timer buys more of the same freeze - it is not announced as a new one
TEST_F(BonusTest, AnotherPlayersTimerExtendsTheFreeze)
{
	int activations{};
	const EventSubscription statusSub{_events->AddListener(Key(Faction::EnemyTeam),
		[&activations](const BonusTimerStatusChangeEvent& event)
		{
			activations += event.isActive ? 1 : 0;
		})};
	_bonusSpawner->SpawnBonus({.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize}, BonusType::Timer);
	const auto first{std::dynamic_pointer_cast<Bonus>(_allObjects.back())};
	_bonusSpawner->SpawnBonus({.x = _tankSize * 3.0, .y = 0.0, .w = _tankSize, .h = _tankSize}, BonusType::Timer);
	const auto second{std::dynamic_pointer_cast<Bonus>(_allObjects.back())};
	ASSERT_NE(first, nullptr);
	ASSERT_NE(second, nullptr);

	first->PickUpBonus(Author::Player1);
	second->PickUpBonus(Author::Player2);

	EXPECT_EQ(activations, 1);
}

// and leaves it driving while nobody has picked it up
TEST_F(BonusTest, TimerNotPickUpEnemyCanMove)
{
	CreatePlayer({.x = 0.0, .y = 0.0});
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = isPressed});

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Timer);

	const auto enemyBot{CreateBot({.x = _tankSize * 2, .y = _tankSize * 2}, Author::Enemy1, Direction::DOWN)};

	const FPoint enemyPos{enemyBot->GetPos()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_NE(enemyPos, enemyBot->GetPos());
}

// the helmet turns the next bullet away
TEST_F(BonusTest, HelmetPickUpAndBulletCantDamageTank)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0})};
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Helmet);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	//NOTE: taken after the pickup - the pickup heals
	const int playerHealth{player->GetHealth()};
	CreateBullet({.x = _tankSize + 1.0, .y = 0.0}, Direction::LEFT, Author::Enemy1);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(playerHealth, player->GetHealth());
}

// and the same bullet goes through without it
TEST_F(BonusTest, HelmetNotPickUpBulletCanDamageTank)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0})};
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = isPressed});
	const int playerHealth{player->GetHealth()};

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Helmet);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	CreateBullet({.x = _tankSize + 1.0, .y = 0.0}, Direction::LEFT, Author::Enemy1);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_NE(playerHealth, player->GetHealth());
}

// the grenade empties the health of every enemy on the field
TEST_F(BonusTest, GrenadePickUpEnemyHealthZero)
{
	CreatePlayer({.x = 0.0, .y = 0.0});
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});

	const auto enemyBot{CreateBot({.x = _tankSize * 2, .y = _tankSize * 2}, Author::Enemy1, Direction::DOWN)};

	EXPECT_EQ(enemyBot->GetHealth(), 100);

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Grenade);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(enemyBot->GetHealth(), 0);
}

// and leaves it full while it lies untouched
TEST_F(BonusTest, GrenadeNotPickUpEnemyHealthFull)
{
	CreatePlayer({.x = 0.0, .y = 0.0});
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = isPressed});

	const auto enemyBot{CreateBot({.x = _tankSize * 2, .y = _tankSize * 2}, Author::Enemy1, Direction::DOWN)};

	EXPECT_EQ(enemyBot->GetHealth(), 100);

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Grenade);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(enemyBot->GetHealth(), 100);
}

// the tank bonus is a life, so the respawn count of the seat grows
TEST_F(BonusTest, TankPickUpExtraLife)
{
	unsigned short respawnActual{3u};
	auto respawnSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		respawnActual = event.respawnCount;
	})};

	CreatePlayer({.x = 0.0, .y = 0.0});
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Tank);

	const unsigned short playerSpawnCount{respawnActual};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(playerSpawnCount, respawnActual);

}

// and stays as it was when the tank drives the other way
TEST_F(BonusTest, TankNotPickUpTierTheSame)
{
	unsigned short respawnActual{3u};
	auto respawnSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		respawnActual = event.respawnCount;
	})};

	CreatePlayer({.x = 0.0, .y = 0.0});
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = isPressed});

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Tank);

	const unsigned short playerSpawnCount{respawnActual};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(playerSpawnCount, respawnActual);

}

// a star is one tier
TEST_F(BonusTest, StarPickUpTierIncrease)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0})};
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Star);

	EXPECT_EQ(player->GetTier(), 1u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(player->GetTier(), 2u);
}

// and none at all until it is taken
TEST_F(BonusTest, StarNotPickUpTierTheSame)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0})};
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = isPressed});

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Star);

	EXPECT_EQ(player->GetTier(), 1u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(player->GetTier(), 1u);
}

// any bonus heals the tank that picked it up, and there is no ceiling - a whole tank grows past its spawn health
TEST_F(BonusTest, PickUpHealsAWholeTankAboveItsSpawnHealth)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0})};
	const int spawnHealth{player->GetHealth()};
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Helmet);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(player->GetHealth(), spawnHealth);
}

// the shovel turns the eagle's brick wall into steel
TEST_F(BonusTest, ShovelPickUpByPlayerThenFortressWallTurnIntoSteelWall)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0})};
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Shovel);

	const ObjRectangle fortressRect{.x = _tankSize + 1.0, .y = 0, .w = _gridSize, .h = _gridSize};
	_events->EmitEvent(SpawnObstacleEvent{.rect = fortressRect, .type = ObstacleType::Fortress});

	EXPECT_NE(dynamic_cast<FortressBrickWall*>(_fortressWall.get()), nullptr);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_NE(dynamic_cast<FortressSteelWall*>(_fortressWall.get()), nullptr);
}

// and heals the eagle at once
TEST_F(BonusTest, ShovelPickUpByPlayerHealsTheEagle)
{
	_events->EmitEvent(SpawnObstacleEvent{.rect = {.x = 0.0, .y = 0.0, .w = _gridSize, .h = _gridSize},
										  .type = ObstacleType::Eagle});
	const std::shared_ptr<BaseObj> eagle{_allObjects.back()};
	const int fullHealth{eagle->GetHealth()};
	eagle->TakeDamage(1u, Author::Enemy1);

	_events->EmitEvent(BonusShovelPickupEvent{.faction = Faction::PlayerTeam});

	EXPECT_EQ(fullHealth, eagle->GetHealth());
}

//TODO: add new tests, that count bricks and check that player can pickup bonus and rebuild fortress and skip if space spawn not available
TEST_F(BonusTest, ShovelNotPickUpByFortressWallTheSame)
{
	CreatePlayer({.x = 0.0, .y = 0.0});
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = isPressed});

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Shovel);

	const ObjRectangle fortressRect{.x = _tankSize + 1.0, .y = 0, .w = _gridSize, .h = _gridSize};
	_events->EmitEvent(SpawnObstacleEvent{.rect = fortressRect, .type = ObstacleType::Fortress});

	EXPECT_NE(dynamic_cast<FortressBrickWall*>(_fortressWall.get()), nullptr);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_NE(dynamic_cast<FortressBrickWall*>(_fortressWall.get()), nullptr);
}

// the control for the ship: water stops a tank that has none
TEST_F(BonusTest, WaterBlocksTankWithoutShip)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0})};
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});

	//NOTE: as wide as the tank - a narrower one it would simply steer around
	const ObjRectangle waterRect{.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _gridSize};
	_events->EmitEvent(SpawnObstacleEvent{.rect = waterRect, .type = ObstacleType::Water});

	constexpr int framesToCrossWater{100};
	for (int frame = 0; frame < framesToCrossWater; ++frame)
	{
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	}

	EXPECT_LE(player->GetBottomSide(), waterRect.y);
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

	EXPECT_GT(player->GetY(), waterRect.y + waterRect.h);
}

// the super star is worth two tiers in one pickup
TEST_F(BonusTest, SuperStarPickUpTierIncreaseTwice)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0})};
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});

	constexpr bool isSuper{true};
	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Star, {},
							  isSuper);

	EXPECT_EQ(player->GetTier(), 1u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(player->GetTier(), 3u);
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

// a bonus the map laid out keeps no clock: with the drop lifetime at nothing, one that fell is swept on
// the next tick and the placed one is still lying there
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

	EXPECT_GT(taker->GetHealth(), 0);
	EXPECT_EQ(otherBot->GetHealth(), 0);
	EXPECT_EQ(player->GetHealth(), 0);
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
