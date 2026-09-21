#include "TestUtils.h"
#include "components/ObstacleSpawner.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/TankPool.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/obstacles/BushTile.h"
#include "entities/obstacles/IceTile.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "enums/InputChannel.h"
#include "gtest/gtest.h"
#include <memory>

//NOTE: bush and ice are read back every frame, not set once at spawn
class TankTerrainTest : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<TankPool> _tankPool{nullptr};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.0 / 60.0};
	Uuid _uuid{};
	double _tankSize{};
	int _tankHealth{100};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_obstacleSpawner = std::make_unique<ObstacleSpawner>(_events, _gameConfig);
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_tankPool = std::make_shared<TankPool>(_events, _allObjects, _gameConfig, _bulletPool);
		_tankSize = _gameConfig.tankSize;
	}

	void TearDown() override {}

	//NOTE: puts the tank into the world too, so the result is a convenience, not the point
	std::shared_ptr<Tank> SpawnPlayerAt(const ObjRectangle rect)
	{
		std::shared_ptr<Tank> tank{TestUtils::CreatePlayer(rect, _tankHealth, Author::Player1, _allObjects, _events,
														   Direction::DOWN, _tankPool, _gameConfig)};
		_allObjects.emplace_back(tank);

		return tank;
	}

	void Tick(const int frames = 1) const
	{
		for (int i = 0; i < frames; ++i)
		{
			_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		}
	}

	void SpawnObstacleArea(const ObjRectangle area, const ObstacleType type) const
	{
		TestUtils::SpawnObstacleArea(_events, _allObjects, area, type, _gameConfig);
	}

	std::shared_ptr<BaseObj> SpawnObstacle(const FPoint pos, const ObstacleType type) const
	{
		return TestUtils::SpawnObstacle(_events, _allObjects, pos, type, _gameConfig);
	}
};

// the control for the two bush cases: on open ground the bar is drawn
TEST_F(TankTerrainTest, HealthBarIsDrawnOnPlainGround)
{
	const std::shared_ptr<Tank> tank{SpawnPlayerAt({.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize})};

	bool isHealthBarDrawn{};
	auto healthBarSub{_events->AddListener([&isHealthBarDrawn](const RenderHealthBarEvent&)
	{
		isHealthBarDrawn = true;
	})};

	Tick();
	_events->EmitEvent(PostDrawEvent{});

	EXPECT_TRUE(isHealthBarDrawn);
	EXPECT_EQ(tank->GetDirection(), Direction::DOWN);
}

// put the tank inside a bush: no bar, so a hidden tank is not given away by its own health
TEST_F(TankTerrainTest, HealthBarIsHiddenWhileTheTankStandsInABush)
{
	SpawnPlayerAt({.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize});
	SpawnObstacle(FPoint{.x = 0.0, .y = 0.0}, ObstacleType::Bush);

	bool isHealthBarDrawn{};
	auto healthBarSub{_events->AddListener([&isHealthBarDrawn](const RenderHealthBarEvent&)
	{
		isHealthBarDrawn = true;
	})};

	Tick();
	_events->EmitEvent(PostDrawEvent{});

	EXPECT_FALSE(isHealthBarDrawn);
}

// burn the bush out from under a standing tank: the bar is back on the next frame
TEST_F(TankTerrainTest, HealthBarComesBackOnceTheBushIsGone)
{
	SpawnPlayerAt({.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize});
	//NOTE: one cell under the tank is all it takes to stand in a bush, and it is the one popped below
	SpawnObstacle(FPoint{.x = 0.0, .y = 0.0}, ObstacleType::Bush);

	bool isHealthBarDrawn{};
	auto healthBarSub{_events->AddListener([&isHealthBarDrawn](const RenderHealthBarEvent&)
	{
		isHealthBarDrawn = true;
	})};

	Tick();
	_events->EmitEvent(PostDrawEvent{});
	ASSERT_FALSE(isHealthBarDrawn);

	_allObjects.pop_back();// the bush burned down

	Tick();
	_events->EmitEvent(PostDrawEvent{});

	EXPECT_TRUE(isHealthBarDrawn);
}

//NOTE: on plain ground the tank is where the key left it - this is the control for the ice test below
TEST_F(TankTerrainTest, TheTankStopsAtOnceOnPlainGround)
{
	const std::shared_ptr<Tank> tank{SpawnPlayerAt({.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize})};

	constexpr int framesUnderPower{20};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = true});
	Tick(framesUnderPower);
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = false});

	const double yOnRelease{tank->GetPos().y};
	EXPECT_GT(yOnRelease, 0.0);

	Tick(framesUnderPower);

	EXPECT_DOUBLE_EQ(tank->GetPos().y, yOnRelease);
}

//NOTE: on ice Move feeds velocity instead of moving - ApplyMoveVelocity spends it later
TEST_F(TankTerrainTest, TheTankKeepsSlidingAfterTheKeyIsReleasedOnIce)
{
	const std::shared_ptr<Tank> tank{SpawnPlayerAt({.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize})};
	SpawnObstacleArea(ObjRectangle{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize * 8.0}, ObstacleType::Ice);

	constexpr int framesUnderPower{20};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = true});
	Tick(framesUnderPower);
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = false});

	const double yOnRelease{tank->GetPos().y};

	Tick();

	EXPECT_GT(tank->GetPos().y, yOnRelease);
}

//NOTE: momentum is per direction, so a turn mid-drift spends both at once
TEST_F(TankTerrainTest, TheTankSlidesDiagonallyWhenTurningWhileDrifting)
{
	const std::shared_ptr<Tank> tank{SpawnPlayerAt({.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize})};
	SpawnObstacleArea(ObjRectangle{.x = 0.0, .y = 0.0, .w = _tankSize * 8.0, .h = _tankSize * 8.0}, ObstacleType::Ice);

	constexpr int framesUnderPower{20};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = true});
	Tick(framesUnderPower);
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = false});

	const double xBeforeTurn{tank->GetPos().x};
	const double yBeforeTurn{tank->GetPos().y};
	ASSERT_DOUBLE_EQ(xBeforeTurn, 0.0);

	_events->EmitEvent(Key(InputChannel::LocalP1), MoveRightEvent{.isPressed = true});
	Tick();

	EXPECT_EQ(tank->GetDirection(), Direction::RIGHT);
	EXPECT_GT(tank->GetPos().x, xBeforeTurn);
	EXPECT_GT(tank->GetPos().y, yBeforeTurn);
}

//NOTE: a tap only turns the sprite - one frame of reverse momentum is spent at once
TEST_F(TankTerrainTest, TurningAroundDoesNotStopTheDriftOnIce)
{
	const std::shared_ptr<Tank> tank{SpawnPlayerAt({.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize})};
	SpawnObstacleArea(ObjRectangle{.x = 0.0, .y = 0.0, .w = _tankSize * 8.0, .h = _tankSize * 8.0}, ObstacleType::Ice);

	constexpr int framesUnderPower{20};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = true});
	Tick(framesUnderPower);
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = false});

	const double yBeforeTap{tank->GetPos().y};
	ASSERT_EQ(tank->GetDirection(), Direction::DOWN);

	_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = true});
	Tick();
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = false});

	Tick(2);

	EXPECT_EQ(tank->GetDirection(), Direction::UP);
	EXPECT_GT(tank->GetPos().y, yBeforeTap);
}
