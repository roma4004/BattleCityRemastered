#include "geometry/Point.h"
#include "TestUtils.h"
#include "components/BulletPool.h"
#include "components/ObstacleSpawner.h"
#include "application/GameConfig.h"
#include "application/ProjectConfig.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/BushTile.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/pawns/Bullet.h"
#include "enums/Direction.h"
#include "gtest/gtest.h"
#include <array>
#include <memory>

class BulletTestAdvanced : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::unique_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	ProjectConfig _projectConfig{"", true};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.0 / 60.0};
	BulletCaliber _caliber{.speed = 300.0, .damage = 1u, .damageRadius = 12.0, .tier = 3u, .size{.x = 6.0, .y = 5.0}};
	Uuid _uuid{};
	double _gridSize{1};
	unsigned short _bulletHealth{1};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_obstacleSpawner = std::make_unique<ObstacleSpawner>(_events, _gameConfig);
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_gridSize = _gameConfig.gridOffset;

		_allObjects.reserve(4);
	}

	void TearDown() override {}

	std::shared_ptr<Bullet> CreateBullet(const FPoint pos, const Direction dir, const Author author)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _caliber.size.x, .h = _caliber.size.y};

		return TestUtils::CreateBullet(rect, _bulletHealth, _bulletPool, _events, _caliber, dir, author);
	}

	std::shared_ptr<BaseObj> SpawnObstacle(const ObjRectangle rect, const ObstacleType type) const
	{
		return TestUtils::SpawnObstacle(_events, _allObjects, rect, type);
	}
};

// the blast is centered where the bullet stopped, so the wall behind stays out of reach at 30 and 144 fps alike
TEST_F(BulletTestAdvanced, BlastSparesTheWallBehindAtAnyFrameRate)
{
	constexpr std::array kFrameRates{30.0, 144.0};
	for (const double fps: kFrameRates)
	{
		SCOPED_TRACE(fps);
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

		const auto nearWall{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 20.0, .w = _gridSize, .h = 4.0},
										  ObstacleType::Brick)};
		const auto farWall{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 34.0, .w = _gridSize, .h = 4.0},
										 ObstacleType::Brick)};

		const unsigned int nearWallHealth{nearWall->GetHealth()};
		const unsigned int farWallHealth{farWall->GetHealth()};

		for (int frame = 0; frame < 40 && nearWall->GetHealth() == nearWallHealth; ++frame)
		{
			_events->EmitEvent(TickUpdateEvent{.deltaTime = 1.0 / fps});
		}

		EXPECT_GT(nearWallHealth, nearWall->GetHealth());
		EXPECT_EQ(farWallHealth, farWall->GetHealth());
	}
}

// a bush never stops a bullet - it burns only in a blast on something solid, and only from tier three
TEST_F(BulletTestAdvanced, BushBurnsInABlastFromTierThree)
{
	CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

	const auto wall{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 20.0, .w = _gridSize, .h = 4.0}, ObstacleType::Brick)};
	const auto bush{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 26.0, .w = _gridSize, .h = _gridSize},
								  ObstacleType::Bush)};

	const unsigned int wallHealth{wall->GetHealth()};
	for (int frame = 0; frame < 20 && wall->GetHealth() == wallHealth; ++frame)
	{
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	}

	EXPECT_GT(wallHealth, wall->GetHealth());
	EXPECT_FALSE(bush->GetIsAlive());
}

// a bush is burnt out only by a tier 3 blast, and survives anything under it
TEST_F(BulletTestAdvanced, BushSurvivesABlastBelowTierThree)
{
	_caliber.tier = 2u;

	CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

	const auto wall{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 20.0, .w = _gridSize, .h = 4.0}, ObstacleType::Brick)};
	const auto bush{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 26.0, .w = _gridSize, .h = _gridSize},
								  ObstacleType::Bush)};

	const unsigned int wallHealth{wall->GetHealth()};
	for (int frame = 0; frame < 20 && wall->GetHealth() == wallHealth; ++frame)
	{
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	}

	EXPECT_GT(wallHealth, wall->GetHealth());
	EXPECT_TRUE(bush->GetIsAlive());
}

// Nothing solid behind it, so the shot flies on and the bush is never in a blast at all
TEST_F(BulletTestAdvanced, TierThreeFliesThroughABushWithoutBurningIt)
{
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
TEST_F(BulletTestAdvanced, WaterAndIceSurviveABlastThatBurnsABush)
{
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
TEST_F(BulletTestAdvanced, ABulletFliesOverWaterAndIce)
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
