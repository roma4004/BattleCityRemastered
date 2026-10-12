#include "geometry/Point.h"
#include "TestUtils.h"
#include "components/ObstacleSpawner.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/TankPool.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/BushTile.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/BulletCaliber.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "gtest/gtest.h"
#include <array>
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
	BulletCaliber _caliber{
			.speed = 300.0, .damage = 1u, .health = 1, .damageRadius = 12.0, .tier = 1u, .size{.x = 6.0, .y = 5.0}};
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

// a brick in the way takes the damage of a shot from any of the four sides
TEST_F(BulletTest, BulletDamagesABrickFromEverySide)
{
	struct Shot final
	{
		FPoint bullet;
		FPoint brick;
		Direction dir;
	};

	constexpr std::array kShots{Shot{.bullet{.x = 0.0, .y = 7.0}, .brick{}, .dir = Direction::UP},
								Shot{.bullet{.x = 7.0, .y = 0.0}, .brick{}, .dir = Direction::LEFT},
								Shot{.bullet{}, .brick{.x = 0.0, .y = 6.0}, .dir = Direction::DOWN},
								Shot{.bullet{}, .brick{.x = 7.0, .y = 0.0}, .dir = Direction::RIGHT}};
	for (const auto& [bulletPos, brickPos, dir]: kShots)
	{
		SCOPED_TRACE(static_cast<int>(dir));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		CreateBullet(bulletPos, dir, Author::Player1);

		const ObjRectangle rect{.x = brickPos.x, .y = brickPos.y, .w = _gridSize, .h = _gridSize};
		const auto brickWall{SpawnObstacle(rect, ObstacleType::Brick)};
		const unsigned int brickWallHealth{brickWall->GetHealth()};

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_GT(brickWallHealth, brickWall->GetHealth());
	}
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

		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		std::ignore = TestUtils::CreateBullet(place(0.0, bulletLength), _bulletHealth, _bulletPool, _events, _caliber,
											  dir, Author::Player1);
		SpawnObstacle(place(bulletLength + 1.0, tileSide), ObstacleType::Brick);

		//NOTE: half a pixel inside the blast around where the bullet's center stops
		auto farTile{SpawnObstacle(place(bulletLength / 2.0 + 1.0 + _caliber.damageRadius - 0.5, tileSide),
								   ObstacleType::Brick)};
		_allObjects.emplace_back(farTile);

		const unsigned int healthBefore{farTile->GetHealth()};

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		return healthBefore - farTile->GetHealth();
	};

	const unsigned int damageShotRight{farTileDamage(Direction::RIGHT)};

	ASSERT_GT(damageShotRight, 0u) << "the tile was out of the blast - equal nothings prove no symmetry";
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

	EXPECT_EQ(enemyBot->GetHealth(), 1u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(enemyBot->GetHealth(), 0u);
}

// shots of one rank meeting take each other's whole stock - both go
TEST_F(BulletTest, SameRankBulletsDestroyEachOther)
{
	const auto bullet{CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1)};
	const auto bullet2{CreateBullet({.x = 0, .y = _caliber.size.y + 1}, Direction::UP, Author::Player2)};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(bullet->GetIsAlive());
	EXPECT_FALSE(bullet2->GetIsAlive());
}

// a heavier shell pays the lighter one's stock out of its own, keeps the rest and flies on through where it was
TEST_F(BulletTest, AHeavierBulletOutlivesTheMeetingAndFliesOn)
{
	BulletCaliber heavyCaliber{_caliber};
	heavyCaliber.health = _caliber.health * 2u;
	const ObjRectangle heavyRect{.x = 0.0, .y = 0.0, .w = heavyCaliber.size.x, .h = heavyCaliber.size.y};
	const auto heavy{TestUtils::CreateBullet(heavyRect, heavyCaliber.health, _bulletPool, _events, heavyCaliber,
											 Direction::DOWN, Author::Player1)};
	const auto light{CreateBullet({.x = 0.0, .y = _caliber.size.y + 1}, Direction::UP, Author::Player2)};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(light->GetIsAlive());
	ASSERT_EQ(heavyCaliber.health - _caliber.health, heavy->GetHealth());

	const FPoint metAt{heavy->GetPos()};
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(metAt.y, heavy->GetPos().y);
}

// a shot with health to spare pays for the brick it breaks and keeps going
TEST_F(BulletTest, BulletPenetratesABrickItCanPayFor)
{
	const auto brickWall{SpawnObstacle({.x = 0.0, .y = 6.0, .w = _gridSize, .h = _gridSize}, ObstacleType::Brick)};
	const unsigned int brickWallHealth{brickWall->GetHealth()};
	_caliber.damage = brickWallHealth;
	_caliber.penetratesWalls = true;
	const ObjRectangle bulletRect{.x = 0.0, .y = 0.0, .w = _caliber.size.x, .h = _caliber.size.y};
	const auto bullet{TestUtils::CreateBullet(bulletRect, brickWallHealth + 1, _bulletPool, _events, _caliber,
											  Direction::DOWN, Author::Player1)};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(brickWall->GetIsAlive());
	EXPECT_TRUE(bullet->GetIsAlive());
	EXPECT_EQ(1u, bullet->GetHealth());
}

// a first-tier shot of any tank but the heavy one goes off at the first layer, whatever it has to spare
TEST_F(BulletTest, BulletThatDoesNotPenetrateGoesOffAtABrickItCouldPayFor)
{
	const auto brickWall{SpawnObstacle({.x = 0.0, .y = 6.0, .w = _gridSize, .h = _gridSize}, ObstacleType::Brick)};
	const unsigned int brickWallHealth{brickWall->GetHealth()};
	_caliber.damage = brickWallHealth;
	const ObjRectangle bulletRect{.x = 0.0, .y = 0.0, .w = _caliber.size.x, .h = _caliber.size.y};
	const auto bullet{TestUtils::CreateBullet(bulletRect, brickWallHealth + 1, _bulletPool, _events, _caliber,
											  Direction::DOWN, Author::Player1)};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(brickWall->GetIsAlive());
	EXPECT_FALSE(bullet->GetIsAlive());
}

// steel below the third tier is not penetrated, whatever the shot has to spare
TEST_F(BulletTest, BulletBelowTierThreeDetonatesAgainstSteel)
{
	const auto steelWall{SpawnObstacle({.x = 0.0, .y = 6.0, .w = _gridSize, .h = _gridSize}, ObstacleType::Steel)};
	const unsigned int steelWallHealth{steelWall->GetHealth()};
	_caliber.penetratesWalls = true;
	const ObjRectangle bulletRect{.x = 0.0, .y = 0.0, .w = _caliber.size.x, .h = _caliber.size.y};
	const auto bullet{TestUtils::CreateBullet(bulletRect, steelWallHealth + 1, _bulletPool, _events, _caliber,
											  Direction::DOWN, Author::Player1)};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(bullet->GetIsAlive());
	EXPECT_EQ(steelWallHealth, steelWall->GetHealth());
}

// from the third tier on steel is paid for like brick
TEST_F(BulletTest, TierThreeBulletPenetratesSteel)
{
	const auto steelWall{SpawnObstacle({.x = 0.0, .y = 6.0, .w = _gridSize, .h = _gridSize}, ObstacleType::Steel)};
	const unsigned int steelWallHealth{steelWall->GetHealth()};
	_caliber.tier = 3u;
	_caliber.damage = steelWallHealth;
	_caliber.penetratesWalls = true;
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

	eagle->TakeDamage(eagle->GetHealth(), Author::Enemy1);

	EXPECT_TRUE(isBaseFinished);
}

// the eagle's wall, on the other hand, breaks like brick
TEST_F(BulletTest, BulletDamagefortressWall)
{
	CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

	const auto fortressWall{SpawnObstacle({.x = 0.0, .y = 6.0, .w = _gridSize, .h = _gridSize},
										  ObstacleType::Fortress)};
	const unsigned int fortressWallHealth{fortressWall->GetHealth()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(fortressWallHealth, fortressWall->GetHealth());
}

// a bush never stops a bullet - it burns in a blast on something solid, and only from tier three
TEST_F(BulletTest, ABushBurnsInABlastOnlyFromTierThree)
{
	struct Blast final
	{
		unsigned short tier{};
		bool isBushStanding{};
	};

	constexpr std::array kBlasts{Blast{.tier = 2u, .isBushStanding = true}, Blast{.tier = 3u, .isBushStanding = false}};
	for (const Blast& blast: kBlasts)
	{
		SCOPED_TRACE(blast.tier);
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		_caliber.tier = blast.tier;
		CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

		const auto wall{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 20.0, .w = _gridSize, .h = 4.0},
									  ObstacleType::Brick)};
		const auto bush{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 26.0, .w = _gridSize, .h = _gridSize},
									  ObstacleType::Bush)};

		const unsigned int wallHealth{wall->GetHealth()};
		for (int frame = 0; frame < 20 && wall->GetHealth() == wallHealth; ++frame)
		{
			_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		}

		EXPECT_GT(wallHealth, wall->GetHealth());
		EXPECT_EQ(bush->GetIsAlive(), blast.isBushStanding);
	}
}

// Nothing solid behind it, so the shot flies on and the bush is never in a blast at all
TEST_F(BulletTest, TierThreeFliesThroughABushWithoutBurningIt)
{
	_caliber.tier = 3u;
	CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

	const auto bush{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 20.0, .w = _gridSize, .h = _gridSize},
								  ObstacleType::Bush)};

	for (int frame = 0; frame < 10; ++frame)
	{
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	}

	EXPECT_TRUE(bush->GetIsAlive());
}

// a tier-three blast burns a bush and digs steel, but the water or ice beside the wall comes through untouched
TEST_F(BulletTest, WaterAndIceSurviveABlastThatBurnsABush)
{
	_caliber.tier = 3u;
	constexpr std::array kTerrains{ObstacleType::Water, ObstacleType::Ice};
	for (const ObstacleType terrain: kTerrains)
	{
		SCOPED_TRACE(static_cast<int>(terrain));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

		const auto wall{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 20.0, .w = _gridSize, .h = 4.0},
									  ObstacleType::Brick)};
		const auto tile{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 26.0, .w = _gridSize, .h = _gridSize}, terrain)};

		const unsigned int wallHealth{wall->GetHealth()};
		for (int frame{0}; frame < 20 && wall->GetHealth() == wallHealth; ++frame)
		{
			_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		}

		EXPECT_GT(wallHealth, wall->GetHealth()) << "the shot never reached the wall";
		EXPECT_TRUE(tile->GetIsAlive());
	}
}

// water and ice are terrain a bullet flies over: the shot crosses the tile and leaves it be
TEST_F(BulletTest, ABulletFliesOverWaterAndIce)
{
	constexpr std::array kTerrains{ObstacleType::Water, ObstacleType::Ice};
	for (const ObstacleType terrain: kTerrains)
	{
		SCOPED_TRACE(static_cast<int>(terrain));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		const auto bullet{CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1)};

		const auto tile{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 6.0, .w = _gridSize, .h = _gridSize}, terrain)};
		const unsigned int tileHealth{tile->GetHealth()};

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_TRUE(bullet->GetIsAlive()) << "the terrain stopped the bullet";
		EXPECT_EQ(tileHealth, tile->GetHealth());
		EXPECT_GT(bullet->GetRect().y, 0.0) << "the bullet did not move";
	}
}
