#include "geometry/Point.h"
#include "TestUtils.h"
#include "components/BulletPool.h"
#include "components/ObstacleSpawner.h"
#include "application/GameConfig.h"
#include "application/ProjectConfig.h"
#include "components/EventSystem.h"
#include "components/events/TimingEvents.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/BushTile.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/pawns/Bullet.h"
#include "enums/Direction.h"
#include "gtest/gtest.h"
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
	BulletCalibre _calibre{.speed = 300.0, .damage = 1u, .damageRadius = 12.0, .tier = 3u, .size{.x = 6.0, .y = 5.0}};
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
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _calibre.size.x, .h = _calibre.size.y};

		return TestUtils::CreateBullet(rect, _bulletHealth, _bulletPool, _events, _calibre, dir, author);
	}

	std::shared_ptr<BaseObj> SpawnObstacle(const ObjRectangle rect, const ObstacleType type) const
	{
		return TestUtils::SpawnObstacle(_events, _allObjects, rect, type);
	}
};

TEST_F(BulletTestAdvanced, BulletTier2CanDestroySteelWall)
{
	const auto bullet{CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1)};

	const ObjRectangle wallRect{.x = 0.0, .y = _calibre.size.y + 1, .w = _gridSize, .h = _gridSize};
	const auto steelWall{SpawnObstacle(wallRect, ObstacleType::Steel)};

	steelWall->SetHealth(1);
	ASSERT_EQ(steelWall->GetHealth(), 1);
	ASSERT_EQ(bullet->GetTier(), 3u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(steelWall->GetHealth(), 0);
}

// The blast is centred where the bullet stopped, so a shot digs the same depth at any frame rate:
// the wall behind the one that was hit stays out of reach at 30 and at 144 frames per second alike
TEST_F(BulletTestAdvanced, BlastSparesTheWallBehindAtThirtyFps)
{
	CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

	const auto nearWall{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 20.0, .w = _gridSize, .h = 4.0},
									  ObstacleType::Brick)};
	const auto farWall{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 34.0, .w = _gridSize, .h = 4.0}, ObstacleType::Brick)};

	const int nearWallHealth{nearWall->GetHealth()};
	const int farWallHealth{farWall->GetHealth()};

	for (int frame = 0; frame < 20 && nearWall->GetHealth() == nearWallHealth; ++frame)
	{
		_events->EmitEvent(TickUpdateEvent{.deltaTime = 1.0 / 30.0});
	}

	EXPECT_GT(nearWallHealth, nearWall->GetHealth());
	EXPECT_EQ(farWallHealth, farWall->GetHealth());
}

TEST_F(BulletTestAdvanced, BlastSparesTheWallBehindAtHundredFortyFourFps)
{
	CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

	const auto nearWall{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 20.0, .w = _gridSize, .h = 4.0},
									  ObstacleType::Brick)};
	const auto farWall{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 34.0, .w = _gridSize, .h = 4.0}, ObstacleType::Brick)};

	const int nearWallHealth{nearWall->GetHealth()};
	const int farWallHealth{farWall->GetHealth()};

	for (int frame = 0; frame < 40 && nearWall->GetHealth() == nearWallHealth; ++frame)
	{
		_events->EmitEvent(TickUpdateEvent{.deltaTime = 1.0 / 144.0});
	}

	EXPECT_GT(nearWallHealth, nearWall->GetHealth());
	EXPECT_EQ(farWallHealth, farWall->GetHealth());
}

// A bush never stops a bullet, so it can only ever be caught by a blast that went off on something
// solid - and only from tier three, the same rule that lets a shot through steel
TEST_F(BulletTestAdvanced, BushBurnsInABlastFromTierThree)
{
	CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

	const auto wall{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 20.0, .w = _gridSize, .h = 4.0}, ObstacleType::Brick)};
	const auto bush{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 26.0, .w = _gridSize, .h = _gridSize},
								  ObstacleType::Bush)};

	const int wallHealth{wall->GetHealth()};
	for (int frame = 0; frame < 20 && wall->GetHealth() == wallHealth; ++frame)
	{
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	}

	EXPECT_GT(wallHealth, wall->GetHealth());
	EXPECT_FALSE(bush->GetIsAlive());
}

TEST_F(BulletTestAdvanced, BushSurvivesABlastBelowTierThree)
{
	_calibre.tier = 2u;

	CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

	const auto wall{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 20.0, .w = _gridSize, .h = 4.0}, ObstacleType::Brick)};
	const auto bush{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 26.0, .w = _gridSize, .h = _gridSize},
								  ObstacleType::Bush)};

	const int wallHealth{wall->GetHealth()};
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

// a tier-three blast burns a bush and digs into steel, but terrain it cannot clear - the water beside the
// wall it hit comes through untouched
TEST_F(BulletTestAdvanced, WaterSurvivesABlastThatBurnsABush)
{
	CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

	const auto wall{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 20.0, .w = _gridSize, .h = 4.0}, ObstacleType::Brick)};
	const auto water{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 26.0, .w = _gridSize, .h = _gridSize},
								   ObstacleType::Water)};

	const int wallHealth{wall->GetHealth()};
	for (int frame{0}; frame < 20 && wall->GetHealth() == wallHealth; ++frame)
	{
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	}

	EXPECT_GT(wallHealth, wall->GetHealth()) << "the shot never reached the wall";
	EXPECT_TRUE(water->GetIsAlive());
}

// the same for ice - the blast spares it the way it spares water
TEST_F(BulletTestAdvanced, IceSurvivesABlastThatBurnsABush)
{
	CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1);

	const auto wall{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 20.0, .w = _gridSize, .h = 4.0}, ObstacleType::Brick)};
	const auto ice{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 26.0, .w = _gridSize, .h = _gridSize}, ObstacleType::Ice)};

	const int wallHealth{wall->GetHealth()};
	for (int frame{0}; frame < 20 && wall->GetHealth() == wallHealth; ++frame)
	{
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	}

	EXPECT_GT(wallHealth, wall->GetHealth()) << "the shot never reached the wall";
	EXPECT_TRUE(ice->GetIsAlive());
}

// ice is terrain a bullet flies over: the shot crosses it and only stops at the wall behind
TEST_F(BulletTestAdvanced, ABulletFliesOverIceWithoutStopping)
{
	const auto bullet{CreateBullet({.x = 0.0, .y = 0.0}, Direction::DOWN, Author::Player1)};

	const auto ice{SpawnObstacle(ObjRectangle{.x = 0.0, .y = 10.0, .w = _gridSize, .h = _gridSize}, ObstacleType::Ice)};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_TRUE(bullet->GetIsAlive()) << "the ice stopped the bullet";
	EXPECT_TRUE(ice->GetIsAlive());
	EXPECT_GT(bullet->GetRect().y, 0.0) << "the bullet did not move";
}
