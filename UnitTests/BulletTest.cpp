#include "enums/Author.h"
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
#include "utils/DirectionUtils.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <ranges>
#include <tuple>
#include <utility>
#include <vector>

// a bullet and whatever it flies into, both placed by hand: one tick moves the shot, then health or position is read
class BulletTest : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<TankPool> _tankPool{nullptr};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.0 / 60.0};
	BulletCaliber _caliber{
			.speed = 300.0, .damage = 1u, .health = 1, .blastRadius = 12.0, .tier = 1u, .size{.x = 6.0, .y = 5.0}};
	double _gridSize{1};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_obstacleSpawner = std::make_unique<ObstacleSpawner>(_events, _gameConfig);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_tankPool = std::make_shared<TankPool>(_events, _allObjects, _gameConfig, _bulletPool);
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_gridSize = _gameConfig.gridOffset;

		_allObjects.reserve(4);
	}

	[[nodiscard]] std::shared_ptr<Bullet> CreateBullet(const FPoint pos, const Direction dir, const Author author) const
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _caliber.size.x, .h = _caliber.size.y};

		return TestUtils::CreateBullet(rect, _bulletPool, _events, _caliber, dir, author);
	}

	[[nodiscard]] std::shared_ptr<Tank> CreatePlayer(const FPoint pos, const Author author,
													 const unsigned int health) const
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};

		return TestUtils::CreatePlayer(rect, health, author, _allObjects, _events, Direction::UP, _tankPool,
									   _gameConfig);
	}

	[[nodiscard]] std::shared_ptr<Tank> CreateBot(const FPoint pos, const Author author,
												   const unsigned int health) const
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};

		return TestUtils::CreateBot(rect, health, author, _allObjects, _events, Direction::UP, _tankPool, _gameConfig);
	}

	[[nodiscard]] std::shared_ptr<BaseObj> SpawnObstacle(const ObjRectangle& rect, const ObstacleType type) const
	{
		return TestUtils::SpawnObstacle(_events, _allObjects, rect, type);
	}
};

// all four directions with room ahead: the shot advances along its axis and holds the other one
TEST_F(BulletTest, BulletMoveInsideScreen)
{
	const FPoint farCorner{.x = static_cast<double>(_gameConfig.battlefieldSize.x) - _caliber.size.x,
						   .y = static_cast<double>(_gameConfig.battlefieldSize.y) - _caliber.size.y};
	const std::array starts{std::pair{FPoint{}, Direction::DOWN}, std::pair{FPoint{}, Direction::RIGHT},
							std::pair{farCorner, Direction::UP}, std::pair{farCorner, Direction::LEFT}};
	for (const auto& [start, dir]: starts)
	{
		SCOPED_TRACE(static_cast<int>(dir));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		const auto bullet{CreateBullet(start, dir, Author::Player1)};

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		const auto [unitX, unitY]{DirectionUtils::Unit(dir)};
		const double alongX{bullet->GetPos().x - start.x};
		const double alongY{bullet->GetPos().y - start.y};
		EXPECT_GT(alongX * unitX + alongY * unitY, 0.0) << "the shot did not move along its direction";
		EXPECT_EQ(unitX == 0.0 ? alongX : alongY, 0.0) << "the shot drifted across its direction";
	}
}

// the same four against the edge of the field, where the shot dies instead of flying on
TEST_F(BulletTest, BulletMoveOutSideScreen)
{
	const FPoint farCorner{.x = static_cast<double>(_gameConfig.battlefieldSize.x) - _caliber.size.x,
						   .y = static_cast<double>(_gameConfig.battlefieldSize.y) - _caliber.size.y};
	const std::array starts{std::pair{farCorner, Direction::DOWN}, std::pair{farCorner, Direction::RIGHT},
							std::pair{FPoint{}, Direction::UP}, std::pair{FPoint{}, Direction::LEFT}};
	for (const auto& [start, dir]: starts)
	{
		SCOPED_TRACE(static_cast<int>(dir));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		const auto bullet{CreateBullet(start, dir, Author::Player1)};

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(start, bullet->GetPos());
	}
}

// a brick in the way takes the damage of a shot from any of the four sides
TEST_F(BulletTest, BulletDamagesABrickFromEverySide)
{
	struct Shot final
	{
		FPoint bullet;
		FPoint brick;
		Direction dir{};
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
		std::ignore = CreateBullet(bulletPos, dir, Author::Player1);

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
		std::ignore = TestUtils::CreateBullet(place(0.0, bulletLength), _bulletPool, _events, _caliber,
											  dir, Author::Player1);
		std::ignore = SpawnObstacle(place(bulletLength + 1.0, tileSide), ObstacleType::Brick);

		//NOTE: half a pixel inside the blast around where the bullet's center stops
		const auto farTile{SpawnObstacle(place(bulletLength / 2.0 + 1.0 + _caliber.blastRadius - 0.5, tileSide),
										 ObstacleType::Brick)};

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
	std::ignore = CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

	const auto enemyBot{CreateBot({.x = 0.0, .y = _caliber.size.y}, Author::Enemy1, 1u)};

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

// shells meet: the lighter deals its damage, the heavier loses that much health and flies on; a tank beside is hit once
TEST_F(BulletTest, AHeavierBulletOutlivesTheMeetingAndFliesOn)
{
	constexpr unsigned int tankHealth{100u};
	const auto beside{CreatePlayer({.x = _caliber.size.x + 1.0, .y = 0.0}, Author::Enemy1, tankHealth)};
	BulletCaliber heavyCaliber{_caliber};
	heavyCaliber.health = _caliber.health * 2u;
	heavyCaliber.damage = _caliber.damage * 10u;
	const ObjRectangle heavyRect{.x = 0.0, .y = 0.0, .w = heavyCaliber.size.x, .h = heavyCaliber.size.y};
	const auto heavy{TestUtils::CreateBullet(heavyRect, _bulletPool, _events, heavyCaliber,
											 Direction::DOWN, Author::Player1)};
	const auto light{CreateBullet({.x = 0.0, .y = _caliber.size.y + 1}, Direction::UP, Author::Player2)};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(light->GetIsAlive());
	EXPECT_EQ(tankHealth - _caliber.damage, beside->GetHealth());
	ASSERT_EQ(heavyCaliber.health - _caliber.health, heavy->GetHealth());

	const FPoint metAt{heavy->GetPos()};
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(metAt.y, heavy->GetPos().y);
}

// a shell with health above the brick's and depth to spare breaks the brick, loses that much health and flies on
TEST_F(BulletTest, BulletPenetratesABrickItCanPayFor)
{
	const auto brickWall{SpawnObstacle({.x = 0.0, .y = 6.0, .w = _gridSize, .h = _gridSize}, ObstacleType::Brick)};
	const unsigned int brickWallHealth{brickWall->GetHealth()};
	_caliber.damage = brickWallHealth;
	_caliber.health = brickWallHealth + 1;
	_caliber.blastRadius = _gameConfig.tankSize;
	const auto bullet{CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1)};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(brickWall->GetIsAlive());
	EXPECT_TRUE(bullet->GetIsAlive());
	EXPECT_EQ(1u, bullet->GetHealth());
}

// a shell breaks only the bricks it touches and deals damage once, deeper for a bigger blast; a tank beside survives
TEST_F(BulletTest, AShellSinksAsDeepAsItsBlastLetsAndDealsDamageOnce)
{
	const double tankSize{_gameConfig.tankSize};
	const double column{tankSize * 4.0};
	const double face{tankSize + _caliber.size.y + 1.0};
	std::vector<std::ptrdiff_t> broken{};
	for (const double radius: {tankSize, tankSize * 1.5})
	{
		SCOPED_TRACE(radius);
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		constexpr unsigned int tankHealth{100u};
		constexpr std::size_t layers{12u};
		//NOTE: beside the wall - in reach of a blast at its face, out of reach of one deep inside
		constexpr double aside{32.0};
		const auto layerAt = [this, column, face](const std::size_t layer)
		{
			const double y{face + _gridSize * static_cast<double>(layer)};

			return SpawnObstacle({.x = column, .y = y, .w = _gridSize, .h = _gridSize}, ObstacleType::Brick);
		};
		const auto wall{std::views::iota(std::size_t{}, layers) | std::views::transform(layerAt)
						| std::ranges::to<std::vector>()};
		const double besideX{column + _caliber.size.x / 2.0 + aside};
		const auto beside{CreatePlayer({.x = besideX, .y = face - tankSize}, Author::Player2, tankHealth)};
		_caliber.damage = wall.front()->GetHealth();
		_caliber.health = _caliber.damage * static_cast<unsigned int>(layers) * 2u;
		_caliber.blastRadius = radius;
		const auto shell{CreateBullet({.x = column, .y = tankSize}, Direction::DOWN, Author::Enemy1)};

		for (int tick{}; tick < 200 && shell->GetIsAlive(); ++tick)
		{
			_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		}

		EXPECT_FALSE(shell->GetIsAlive());
		EXPECT_TRUE(wall.back()->GetIsAlive());
		EXPECT_EQ(tankHealth, beside->GetHealth());
		broken.push_back(std::ranges::count_if(wall, [](const auto& layer) { return !layer->GetIsAlive(); }));
	}

	EXPECT_LT(broken.front(), broken.back());
}

// steel below the third tier is not penetrated, whatever the shot has to spare
TEST_F(BulletTest, BulletBelowTierThreeDealsDamageAgainstSteel)
{
	const auto steelWall{SpawnObstacle({.x = 0.0, .y = 6.0, .w = _gridSize, .h = _gridSize}, ObstacleType::Steel)};
	const unsigned int steelWallHealth{steelWall->GetHealth()};
	_caliber.health = steelWallHealth + 1;
	_caliber.blastRadius = _gameConfig.tankSize;
	const auto bullet{CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1)};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(bullet->GetIsAlive());
	EXPECT_EQ(steelWallHealth, steelWall->GetHealth());
}

// from the third tier on steel is penetrated like brick
TEST_F(BulletTest, TierThreeBulletPenetratesSteel)
{
	const auto steelWall{SpawnObstacle({.x = 0.0, .y = 6.0, .w = _gridSize, .h = _gridSize}, ObstacleType::Steel)};
	const unsigned int steelWallHealth{steelWall->GetHealth()};
	_caliber.tier = 3u;
	_caliber.damage = steelWallHealth;
	_caliber.health = steelWallHealth + 1;
	_caliber.blastRadius = _gameConfig.tankSize;
	const auto bullet{CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1)};

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
	std::ignore = CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Enemy1);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	ASSERT_TRUE(eagle->GetIsAlive());
	EXPECT_FALSE(isBaseFinished);

	eagle->TakeDamage(eagle->GetHealth(), Author::Enemy1);

	EXPECT_TRUE(isBaseFinished);
}

// the eagle's wall, on the other hand, breaks like brick
TEST_F(BulletTest, BulletDamagefortressWall)
{
	std::ignore = CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

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

	for (const Blast& blast: {Blast{.tier = 2u, .isBushStanding = true}, Blast{.tier = 3u, .isBushStanding = false}})
	{
		SCOPED_TRACE(blast.tier);
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		_caliber.tier = blast.tier;
		std::ignore = CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

		const auto wall{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 20.0, .w = _gridSize, .h = 4.0},
									  ObstacleType::Brick)};
		const auto bush{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 26.0, .w = _gridSize, .h = _gridSize},
									  ObstacleType::Bush)};

		const unsigned int wallHealth{wall->GetHealth()};
		for (int frame{}; frame < 20 && wall->GetHealth() == wallHealth; ++frame)
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
	std::ignore = CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

	const auto bush{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 20.0, .w = _gridSize, .h = _gridSize},
								  ObstacleType::Bush)};

	for (int frame{}; frame < 10; ++frame)
	{
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	}

	EXPECT_TRUE(bush->GetIsAlive());
}

// a tier-three blast burns a bush and digs steel, but the water or ice beside the wall comes through untouched
TEST_F(BulletTest, WaterAndIceSurviveABlastThatBurnsABush)
{
	_caliber.tier = 3u;
	for (const ObstacleType terrain: {ObstacleType::Water, ObstacleType::Ice})
	{
		SCOPED_TRACE(static_cast<int>(terrain));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		std::ignore = CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

		const auto wall{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 20.0, .w = _gridSize, .h = 4.0},
									  ObstacleType::Brick)};
		const auto tile{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 26.0, .w = _gridSize, .h = _gridSize}, terrain)};

		const unsigned int wallHealth{wall->GetHealth()};
		for (int frame{}; frame < 20 && wall->GetHealth() == wallHealth; ++frame)
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
	for (const ObstacleType terrain: {ObstacleType::Water, ObstacleType::Ice})
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
