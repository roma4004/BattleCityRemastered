#include "geometry/Point.h"
#include "TestUtils.h"
#include "application/GameConfig.h"
#include "application/ProjectConfig.h"
#include "components/EventSystem.h"
#include "components/events/TimingEvents.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/pawns/Bullet.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include "enums/Faction.h"
#include <memory>

class BulletTestAdvanced : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	ProjectConfig _projectConfig{"", true};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.0 / 60.0};
	BulletCalibre _calibre{.speed = 300.0, .damage = 1u, .damageRadius = 12.0, .tier = 3u, .size{.x = 6.0, .y = 5.0}};
	Uuid _uuid{};
	double _gridSize{1};
	unsigned short _bulletHealth{1};
	GameMode _gameMode{GameMode::OnePlayer};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_gridSize = _gameConfig.gridOffset;

		_allObjects.reserve(4);

		const ObjRectangle rectBullet{.x = 0.0, .y = 0.0, .w = _calibre.size.x, .h = _calibre.size.y};
		std::shared_ptr<Bullet> bullet =
				TestUtils::CreateBullet(
						rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::PlayerTeam, _allObjects,
						_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
		_allObjects.emplace_back(bullet);
	}

	void TearDown() override
	{
	}
};

TEST_F(BulletTestAdvanced, BulletTier2CanDestroySteelWall)
{
	if (const Bullet* bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
	{
		// spawn BrickWall
		ObjRectangle wallRect = {.x = 0.0, .y = _calibre.size.y + 1, .w = _gridSize, .h = _gridSize};
		auto steelWall = std::make_shared<SteelWall>(wallRect, _events, _uuid, _gameMode);
		_allObjects.emplace_back(steelWall);

		steelWall->SetHealth(1);
		EXPECT_EQ(steelWall->GetHealth(), 1);
		EXPECT_EQ(bullet->GetTier(), 3u);

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(steelWall->GetHealth(), 0);

		return;
	}

	EXPECT_FALSE(true);
}

// The blast is centred where the bullet stopped, so a shot digs the same depth at any frame rate:
// the wall behind the one that was hit stays out of reach at 30 and at 144 frames per second alike
TEST_F(BulletTestAdvanced, BlastSparesTheWallBehindAtThirtyFps)
{
	auto nearWall = std::make_shared<BrickWall>(ObjRectangle{.x = 0.0, .y = 20.0, .w = _gridSize, .h = 4.0},
												_events, _uuid, _gameMode);
	auto farWall = std::make_shared<BrickWall>(ObjRectangle{.x = 0.0, .y = 34.0, .w = _gridSize, .h = 4.0},
											   _events, _uuid, _gameMode);
	_allObjects.emplace_back(nearWall);
	_allObjects.emplace_back(farWall);

	const int nearWallHealth = nearWall->GetHealth();
	const int farWallHealth = farWall->GetHealth();

	for (int frame = 0; frame < 20 && nearWall->GetHealth() == nearWallHealth; ++frame)
	{
		_events->EmitEvent(TickUpdateEvent{.deltaTime = 1.0 / 30.0});
	}

	EXPECT_GT(nearWallHealth, nearWall->GetHealth());
	EXPECT_EQ(farWallHealth, farWall->GetHealth());
}

TEST_F(BulletTestAdvanced, BlastSparesTheWallBehindAtHundredFortyFourFps)
{
	auto nearWall = std::make_shared<BrickWall>(ObjRectangle{.x = 0.0, .y = 20.0, .w = _gridSize, .h = 4.0},
												_events, _uuid, _gameMode);
	auto farWall = std::make_shared<BrickWall>(ObjRectangle{.x = 0.0, .y = 34.0, .w = _gridSize, .h = 4.0},
											   _events, _uuid, _gameMode);
	_allObjects.emplace_back(nearWall);
	_allObjects.emplace_back(farWall);

	const int nearWallHealth = nearWall->GetHealth();
	const int farWallHealth = farWall->GetHealth();

	for (int frame = 0; frame < 40 && nearWall->GetHealth() == nearWallHealth; ++frame)
	{
		_events->EmitEvent(TickUpdateEvent{.deltaTime = 1.0 / 144.0});
	}

	EXPECT_GT(nearWallHealth, nearWall->GetHealth());
	EXPECT_EQ(farWallHealth, farWall->GetHealth());
}
