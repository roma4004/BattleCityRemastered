#include "geometry/Point.h"
#include "TestUtils.h"
#include "components/ObstacleSpawner.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/TankPool.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/FortressWalls.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "enums/Faction.h"
#include "gtest/gtest.h"
#include <memory>

// a bullet and whatever it flies into, both placed by hand: one tick moves the shot, then health or position is read
class BulletTest : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.0 / 60.0};
	BulletCaliber _caliber{.speed = 300.0, .damage = 1u, .damageRadius = 12.0, .tier = 1u, .size{.x = 6.0, .y = 5.0}};
	Uuid _uuid{};
	double _gridSize{1};
	unsigned short _bulletHealth{1u};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_obstacleSpawner = std::make_unique<ObstacleSpawner>(_events, _gameConfig);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_gridSize = _gameConfig.gridOffset;

		_allObjects.reserve(4);
	}

	void TearDown() override {}

	std::shared_ptr<Bullet> CreateBullet(const FPoint pos, const Direction dir, const Author author)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _caliber.size.x, .h = _caliber.size.y};
		auto bullet{TestUtils::CreateBullet(rect, _bulletHealth, _bulletPool, _events, _caliber, dir,
											author)};

		return bullet;
	}

	std::shared_ptr<BaseObj> SpawnObstacle(const ObjRectangle rect, const ObstacleType type) const
	{
		return TestUtils::SpawnObstacle(_events, _allObjects, rect, type);
	}
};

// a placement bypassing flight, the way the pool hands a bullet out
TEST_F(BulletTest, BulletSetPos)
{
	const auto bullet{CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1)};

	const auto windowWidth{static_cast<double>(_gameConfig.battlefieldSize.x)};
	const auto windowHeight{static_cast<double>(_gameConfig.battlefieldSize.y)};
	bullet->SetPos({.x = windowWidth, .y = windowHeight});

	EXPECT_EQ(bullet->GetPos(), (FPoint{.x = windowWidth, .y = windowHeight}));
}

// and the same for the way it faces
TEST_F(BulletTest, BulletSetDirection)
{
	const auto bullet{CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1)};

	const Direction startDirection{bullet->GetDirection()};

	bullet->SetDirection(Direction::UP);

	EXPECT_NE(startDirection, bullet->GetDirection());
	EXPECT_EQ(Direction::UP, bullet->GetDirection());
}

// all four directions with room ahead: the shot advances along its axis and holds the other one
TEST_F(BulletTest, BulletMoveInsideScreen)
{
	const auto bullet{CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1)};

	{
		bullet->SetDirection(Direction::DOWN);
		const FPoint bulletStartPos{bullet->GetPos()};
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		const FPoint bulletEndPos{bullet->GetPos()};
		EXPECT_LT(bulletStartPos.y, bulletEndPos.y);
		EXPECT_EQ(bulletStartPos.x, bulletEndPos.x);
	}
	{
		bullet->SetDirection(Direction::RIGHT);
		const FPoint bulletStartPos{bullet->GetPos()};
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		const FPoint bulletEndPos{bullet->GetPos()};
		EXPECT_LT(bulletStartPos.x, bulletEndPos.x);
		EXPECT_EQ(bulletStartPos.y, bulletEndPos.y);
	}

	const auto windowWidth{static_cast<double>(_gameConfig.battlefieldSize.x)};
	const auto windowHeight{static_cast<double>(_gameConfig.battlefieldSize.y)};

	bullet->SetPos({.x = windowWidth - _caliber.size.x, .y = windowHeight - _caliber.size.y});
	{
		bullet->SetDirection(Direction::UP);
		const FPoint bulletStartPos{bullet->GetPos()};
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		const FPoint bulletEndPos{bullet->GetPos()};
		EXPECT_GT(bulletStartPos.y, bulletEndPos.y);
		EXPECT_EQ(bulletStartPos.x, bulletEndPos.x);
	}
	{
		bullet->SetDirection(Direction::LEFT);
		const FPoint bulletStartPos{bullet->GetPos()};
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		const FPoint bulletEndPos{bullet->GetPos()};
		EXPECT_GT(bulletStartPos.x, bulletEndPos.x);
		EXPECT_EQ(bulletStartPos.y, bulletEndPos.y);
	}
}

// the same four against the edge of the field, where the shot dies instead of flying on
TEST_F(BulletTest, BulletMoveOutSideScreen)
{
	const auto bullet{CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1)};

	const auto windowWidth{static_cast<double>(_gameConfig.battlefieldSize.x)};
	const auto windowHeight{static_cast<double>(_gameConfig.battlefieldSize.y)};

	bullet->SetPos({.x = windowWidth - _caliber.size.x, .y = windowHeight - _caliber.size.y});
	{
		bullet->SetDirection(Direction::DOWN);
		const FPoint bulletStartPos{bullet->GetPos()};
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		EXPECT_EQ(bulletStartPos, bullet->GetPos());
	}
	{
		bullet->SetDirection(Direction::RIGHT);
		const FPoint bulletStartPos{bullet->GetPos()};
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		EXPECT_EQ(bulletStartPos, bullet->GetPos());
	}

	bullet->SetPos({.x = 0.0, .y = 0.0});
	{
		bullet->SetDirection(Direction::UP);
		const FPoint bulletStartPos{bullet->GetPos()};
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		EXPECT_EQ(bulletStartPos, bullet->GetPos());
	}
	{
		bullet->SetDirection(Direction::LEFT);
		const FPoint bulletStartPos{bullet->GetPos()};
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		EXPECT_EQ(bulletStartPos, bullet->GetPos());
	}
}

// a brick in the way takes the damage of a shot from below
TEST_F(BulletTest, BulletDamageBrickWhenMoveUp)
{
	CreateBullet({.x = 0.0, .y = 7.0}, Direction::UP, Author::Player1);

	const ObjRectangle rect{.x = 0, .y = 0, .w = _gridSize, .h = _gridSize};
	auto brickWall{SpawnObstacle(rect, ObstacleType::Brick)};
	_allObjects.emplace_back(brickWall);

	const int brickWallHealth{brickWall->GetHealth()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(brickWallHealth, brickWall->GetHealth());
}

// and from the right
TEST_F(BulletTest, BulletDamageBrickWhenMoveLeft)
{
	CreateBullet({.x = 7.0, .y = 0.0}, Direction::LEFT, Author::Player1);

	const ObjRectangle rect{.x = 0, .y = 0, .w = _gridSize, .h = _gridSize};
	auto brickWall{SpawnObstacle(rect, ObstacleType::Brick)};
	_allObjects.emplace_back(brickWall);

	const int brickWallHealth{brickWall->GetHealth()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(brickWallHealth, brickWall->GetHealth());
}

// from above
TEST_F(BulletTest, BulletDamageBrickWhenMoveDown)
{
	CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

	const ObjRectangle rect{.x = 0.0, .y = 6.0, .w = _gridSize, .h = _gridSize};
	auto brickWall{SpawnObstacle(rect, ObstacleType::Brick)};
	_allObjects.emplace_back(brickWall);

	const int brickWallHealth{brickWall->GetHealth()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(brickWallHealth, brickWall->GetHealth());
}

// and from the left
TEST_F(BulletTest, BulletDamageBrickWhenMoveRight)
{
	CreateBullet({.x = 0.0, .y = 0.0}, Direction::RIGHT, Author::Player1);

	const ObjRectangle rect{.x = 7.0, .y = 0.0, .w = _gridSize, .h = _gridSize};
	auto brickWall{SpawnObstacle(rect, ObstacleType::Brick)};
	_allObjects.emplace_back(brickWall);

	const int brickWallHealth{brickWall->GetHealth()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(brickWallHealth, brickWall->GetHealth());
}

// the same tile at the edge of the blast, shot from each of the four sides - one radius, not four
TEST_F(BulletTest, BulletBlowRadiusIsDirectionSymmetric)
{
	constexpr double mirrorAxis{200.0};
	constexpr double tileSide{2.0};

	auto farTileDamage = [this](const Direction dir)
	{
		const bool isHorizontal{dir == Direction::LEFT || dir == Direction::RIGHT};
		const bool isMirrored{dir == Direction::LEFT || dir == Direction::UP};
		const double bulletLength{isHorizontal ? _caliber.size.x : _caliber.size.y};

		// offset and length are measured along the shot axis, starting at the bullet's back edge
		auto place = [&](const double offset, const double length)
		{
			const double along{isMirrored ? mirrorAxis - offset - length : offset};

			return isHorizontal
					   ? ObjRectangle{.x = along, .y = 0.0, .w = length, .h = tileSide}
					   : ObjRectangle{.x = 0.0, .y = along, .w = tileSide, .h = length};
		};

		_allObjects.clear();
		std::ignore = TestUtils::CreateBullet(place(0.0, bulletLength), _bulletHealth, _bulletPool, _events, _caliber,
											  dir, Author::Player1);
		SpawnObstacle(place(bulletLength + 1.0, tileSide), ObstacleType::Brick);

		// just outside the radius measured from the bullet's leading edge, just inside it from the bullet's center
		auto farTile{SpawnObstacle(place(bulletLength + 1.0 + _caliber.damageRadius, tileSide), ObstacleType::Brick)};
		_allObjects.emplace_back(farTile);

		const int healthBefore{farTile->GetHealth()};

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		return healthBefore - farTile->GetHealth();
	};

	const int damageShotRight{farTileDamage(Direction::RIGHT)};

	EXPECT_EQ(damageShotRight, farTileDamage(Direction::LEFT));
	EXPECT_EQ(damageShotRight, farTileDamage(Direction::DOWN));
	EXPECT_EQ(damageShotRight, farTileDamage(Direction::UP));
}

// a tank takes the hit like any other target
TEST_F(BulletTest, BulletDamageTank)
{
	CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

	constexpr unsigned short tankHealth{1u};
	const auto tankPool{std::make_shared<TankPool>(_events, _allObjects, _gameConfig, _bulletPool)};
	const ObjRectangle rectEnemy{.x = 0.0,
								 .y = _caliber.size.y,
								 .w = _gameConfig.tankSize,
								 .h = _gameConfig.tankSize};
	const std::shared_ptr<Tank> enemyBot{TestUtils::CreateBot(rectEnemy, tankHealth, Author::Enemy1, _allObjects,
															  _events, Direction::UP, tankPool, _gameConfig)};

	EXPECT_EQ(enemyBot->GetHealth(), 1);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(enemyBot->GetHealth(), 0);
}

// two shots meeting kill each other
TEST_F(BulletTest, BulletToBulletDamageEachOther)
{
	const auto bullet{CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1)};
	const auto bullet2{CreateBullet({.x = 0, .y = _caliber.size.y + 1}, Direction::UP, Author::Player2)};

	const int bulletHealth{bullet->GetHealth()};
	const int bullet2Health{bullet2->GetHealth()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(bulletHealth, bullet->GetHealth());
	EXPECT_GT(bullet2Health, bullet2->GetHealth());
}

// the heavier shot takes the lighter one's damage and keeps the rest of its health
TEST_F(BulletTest, HeavierBulletOutlivesTheMeeting)
{
	constexpr int heavyHealth{2};
	const ObjRectangle heavyRect{.x = 0.0, .y = 0.0, .w = _caliber.size.x, .h = _caliber.size.y};
	const auto heavy{TestUtils::CreateBullet(heavyRect, heavyHealth, _bulletPool, _events, _caliber, Direction::DOWN,
											 Author::Player1)};
	const auto light{CreateBullet({.x = 0.0, .y = _caliber.size.y + 1}, Direction::UP, Author::Player2)};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(light->GetIsAlive());
	EXPECT_EQ(heavyHealth - static_cast<int>(_caliber.damage), heavy->GetHealth());
}

// and flies on through where the lighter one was
TEST_F(BulletTest, SurvivingBulletFliesOn)
{
	const ObjRectangle heavyRect{.x = 0.0, .y = 0.0, .w = _caliber.size.x, .h = _caliber.size.y};
	const auto heavy{TestUtils::CreateBullet(heavyRect, 2, _bulletPool, _events, _caliber, Direction::DOWN,
											 Author::Player1)};
	CreateBullet({.x = 0.0, .y = _caliber.size.y + 1}, Direction::UP, Author::Player2);

	const FPoint heavyStartPos{heavy->GetPos()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(heavyStartPos.y, heavy->GetPos().y);
}

// a shot with health to spare pays for the brick it breaks and keeps going
TEST_F(BulletTest, BulletSinksIntoABrickItCanPayFor)
{
	const auto brickWall{SpawnObstacle({.x = 0.0, .y = 6.0, .w = _gridSize, .h = _gridSize}, ObstacleType::Brick)};
	const int brickWallHealth{brickWall->GetHealth()};
	_caliber.damage = static_cast<unsigned int>(brickWallHealth);
	const ObjRectangle bulletRect{.x = 0.0, .y = 0.0, .w = _caliber.size.x, .h = _caliber.size.y};
	const auto bullet{TestUtils::CreateBullet(bulletRect, brickWallHealth + 1, _bulletPool, _events, _caliber,
											  Direction::DOWN, Author::Player1)};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(brickWall->GetIsAlive());
	EXPECT_TRUE(bullet->GetIsAlive());
	EXPECT_EQ(1, bullet->GetHealth());
}

// a shell met inside the wall finishes the one sinking into it, and that one still goes off
TEST_F(BulletTest, BulletFinishedInsideAWallStillExplodes)
{
	int explosions{};
	const EventSubscription explosionSub{_events->AddListener(
			[&explosions](const AnimationCreateBulletExplosionEvent&) { ++explosions; })};
	const auto brickWall{SpawnObstacle({.x = 0.0, .y = 6.0, .w = _gridSize, .h = _gridSize}, ObstacleType::Brick)};
	const int brickWallHealth{brickWall->GetHealth()};
	_caliber.damage = static_cast<unsigned int>(brickWallHealth);
	const ObjRectangle bulletRect{.x = 0.0, .y = 0.0, .w = _caliber.size.x, .h = _caliber.size.y};
	const auto bullet{TestUtils::CreateBullet(bulletRect, brickWallHealth + 1, _bulletPool, _events, _caliber,
											  Direction::DOWN, Author::Player1)};
	CreateBullet({.x = _caliber.size.x + 1.0, .y = 6.0}, Direction::RIGHT, Author::Enemy1);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	ASSERT_FALSE(brickWall->GetIsAlive());
	ASSERT_FALSE(bullet->GetIsAlive());
	EXPECT_EQ(1, explosions);
}

// steel below the third tier is not sunk into, whatever the shot has to spare
TEST_F(BulletTest, BulletBelowTierThreeDetonatesAgainstSteel)
{
	const auto steelWall{SpawnObstacle({.x = 0.0, .y = 6.0, .w = _gridSize, .h = _gridSize}, ObstacleType::Steel)};
	const int steelWallHealth{steelWall->GetHealth()};
	const ObjRectangle bulletRect{.x = 0.0, .y = 0.0, .w = _caliber.size.x, .h = _caliber.size.y};
	const auto bullet{TestUtils::CreateBullet(bulletRect, steelWallHealth + 1, _bulletPool, _events, _caliber,
											  Direction::DOWN, Author::Player1)};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(bullet->GetIsAlive());
	EXPECT_EQ(steelWallHealth, steelWall->GetHealth());
}

// from the third tier on steel is paid for like brick
TEST_F(BulletTest, TierThreeBulletSinksIntoSteel)
{
	const auto steelWall{SpawnObstacle({.x = 0.0, .y = 6.0, .w = _gridSize, .h = _gridSize}, ObstacleType::Steel)};
	const int steelWallHealth{steelWall->GetHealth()};
	_caliber.tier = 3u;
	_caliber.damage = static_cast<unsigned int>(steelWallHealth);
	const ObjRectangle bulletRect{.x = 0.0, .y = 0.0, .w = _caliber.size.x, .h = _caliber.size.y};
	const auto bullet{TestUtils::CreateBullet(bulletRect, steelWallHealth + 1, _bulletPool, _events, _caliber,
											  Direction::DOWN, Author::Player1)};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(steelWall->GetIsAlive());
	EXPECT_TRUE(bullet->GetIsAlive());
}

// the eagle outlives a shot - the base falls only when its health is gone
TEST_F(BulletTest, EagleFallsOnlyWhenItsHealthIsGone)
{
	bool isBaseFinished{};
	const EventSubscription baseSub{_events->AddListener([&isBaseFinished](const PlayersBaseFinishedEvent&)
	{
		isBaseFinished = true;
	})};
	const auto eagle{SpawnObstacle({.x = 0.0, .y = 6.0, .w = _gridSize, .h = _gridSize}, ObstacleType::Eagle)};
	CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Enemy1);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	ASSERT_TRUE(eagle->GetIsAlive());
	EXPECT_FALSE(isBaseFinished);

	eagle->TakeDamage(static_cast<unsigned int>(eagle->GetHealth()), Author::Enemy1);

	EXPECT_TRUE(isBaseFinished);
}

// the shovel heals a standing eagle, not one shot down in the same tick
TEST_F(BulletTest, ShovelDoesNotHealAFallenEagle)
{
	const auto eagle{SpawnObstacle({.x = 0.0, .y = 6.0, .w = _gridSize, .h = _gridSize}, ObstacleType::Eagle)};
	eagle->TakeDamage(static_cast<unsigned int>(eagle->GetHealth()), Author::Enemy1);

	_events->EmitEvent(BonusShovelStatusChangeEvent{.faction = Faction::PlayerTeam, .isActive = true});

	EXPECT_FALSE(eagle->GetIsAlive());
	EXPECT_GE(0, eagle->GetHealth());
}

// steel swallows a tier 1 shot whole
TEST_F(BulletTest, BulletCantDamageSteelWall)
{
	const auto bullet{CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1)};

	const ObjRectangle rect{.x = 0.0, .y = 6.0, .w = _gridSize, .h = _gridSize};
	auto steelWall{SpawnObstacle(rect, ObstacleType::Steel)};
	_allObjects.emplace_back(steelWall);

	const int bulletHealth{bullet->GetHealth()};
	const int steelWallHealth{steelWall->GetHealth()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(bulletHealth, bullet->GetHealth());
	EXPECT_EQ(steelWallHealth, steelWall->GetHealth());
}

// and water is flown over rather than hit
TEST_F(BulletTest, BulletCantDamageWater)
{
	const auto bullet{CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1)};

	const ObjRectangle rect{.x = 0.0, .y = 6.0, .w = _gridSize, .h = _gridSize};
	auto waterTile{SpawnObstacle(rect, ObstacleType::Water)};
	_allObjects.emplace_back(waterTile);

	const int bulletHealth{bullet->GetHealth()};
	const int waterTileHealth{waterTile->GetHealth()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(bulletHealth, bullet->GetHealth());
	EXPECT_EQ(waterTileHealth, waterTile->GetHealth());
}

// the eagle's wall, on the other hand, breaks like brick
TEST_F(BulletTest, BulletDamagefortressWall)
{
	CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

	constexpr ObjRectangle rect{.x = 0.0, .y = 6.0, .w = 36, .h = 36};
	auto fortressWall{std::make_shared<FortressBrickWall>(rect, _events, _uuid, _gameConfig)};
	_allObjects.emplace_back(fortressWall);

	fortressWall->SetHealth(1);
	EXPECT_EQ(fortressWall->GetHealth(), 1);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(fortressWall->GetIsAlive());
}

// the shot spends itself on what it hits, so it dies with the wall
TEST_F(BulletTest, BulletHaveSelfDamageWhenHit)
{
	const auto bullet{CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1)};

	constexpr ObjRectangle rect{.x = 0.0, .y = 6.0, .w = 36, .h = 36};
	auto brickWall{SpawnObstacle(rect, ObstacleType::Brick)};
	_allObjects.emplace_back(brickWall);

	bullet->SetHealth(1);
	EXPECT_EQ(bullet->GetHealth(), 1);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(bullet->GetHealth(), 0);
}
