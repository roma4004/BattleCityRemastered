#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/obstacles/BushTile.h"
#include "entities/obstacles/IceTile.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/InputChannel.h"
#include "gtest/gtest.h"
#include "enums/Faction.h"
#include <memory>

//NOTE: bush and ice are read back every frame, not set once at spawn
class TankTerrainTest : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.0 / 60.0};
	Uuid _uuid{};
	double _tankSize{};
	double _tankSpeed{142.0};
	int _tankHealth{100};
	GameMode _gameMode{GameMode::OnePlayer};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_tankSize = _gameConfig.gridOffset * 3.0;
	}

	void TearDown() override {}

	//NOTE: puts the tank into the world too, so the result is a convenience, not the point
	std::shared_ptr<Tank> SpawnPlayerAt(const ObjRectangle rect)
	{
		std::shared_ptr<Tank> tank =
				TestUtils::CreatePlayer(rect, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects,
										_events, 1u, _tankSpeed, Direction::DOWN, _bulletPool, _gameConfig);
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
};

TEST_F(TankTerrainTest, HealthBarIsDrawnOnPlainGround)
{
	const std::shared_ptr<Tank> tank = SpawnPlayerAt({.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize});

	bool isHealthBarDrawn{false};
	auto healthBarSub = _events->AddListener([&isHealthBarDrawn](const RenderHealthBarEvent&)
	{
		isHealthBarDrawn = true;
	});

	Tick();
	_events->EmitEvent(PostDrawEvent{});

	EXPECT_TRUE(isHealthBarDrawn);
	EXPECT_EQ(tank->GetDirection(), Direction::DOWN);
}

TEST_F(TankTerrainTest, HealthBarIsHiddenWhileTheTankStandsInABush)
{
	SpawnPlayerAt({.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize});
	_allObjects.emplace_back(std::make_shared<BushTile>(
			ObjRectangle{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize}, _events, _uuid, _gameMode));

	bool isHealthBarDrawn{false};
	auto healthBarSub = _events->AddListener([&isHealthBarDrawn](const RenderHealthBarEvent&)
	{
		isHealthBarDrawn = true;
	});

	Tick();
	_events->EmitEvent(PostDrawEvent{});

	EXPECT_FALSE(isHealthBarDrawn);
}

//NOTE: the flag is re-read every frame, so taking the bush away is enough to bring the bar back
TEST_F(TankTerrainTest, HealthBarComesBackOnceTheBushIsGone)
{
	SpawnPlayerAt({.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize});
	_allObjects.emplace_back(std::make_shared<BushTile>(
			ObjRectangle{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize}, _events, _uuid, _gameMode));

	bool isHealthBarDrawn{false};
	auto healthBarSub = _events->AddListener([&isHealthBarDrawn](const RenderHealthBarEvent&)
	{
		isHealthBarDrawn = true;
	});

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
	const std::shared_ptr<Tank> tank = SpawnPlayerAt({.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize});

	constexpr int framesUnderPower{20};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = true});
	Tick(framesUnderPower);
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = false});

	const double yOnRelease = tank->GetPos().y;
	EXPECT_GT(yOnRelease, 0.0);

	Tick(framesUnderPower);

	EXPECT_DOUBLE_EQ(tank->GetPos().y, yOnRelease);
}

//NOTE: on ice Move feeds velocity instead of moving - ApplyMoveVelocity spends it later
TEST_F(TankTerrainTest, TheTankKeepsSlidingAfterTheKeyIsReleasedOnIce)
{
	const std::shared_ptr<Tank> tank = SpawnPlayerAt({.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize});
	_allObjects.emplace_back(std::make_shared<IceTile>(
			ObjRectangle{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize * 8.0}, _events, _uuid, _gameMode));

	constexpr int framesUnderPower{20};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = true});
	Tick(framesUnderPower);
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = false});

	const double yOnRelease = tank->GetPos().y;

	Tick();

	EXPECT_GT(tank->GetPos().y, yOnRelease);
}

//NOTE: momentum is per direction, so a turn mid-drift spends both at once
TEST_F(TankTerrainTest, TheTankSlidesDiagonallyWhenTurningWhileDrifting)
{
	const std::shared_ptr<Tank> tank = SpawnPlayerAt({.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize});
	_allObjects.emplace_back(std::make_shared<IceTile>(
			ObjRectangle{.x = 0.0, .y = 0.0, .w = _tankSize * 8.0, .h = _tankSize * 8.0}, _events, _uuid, _gameMode));

	constexpr int framesUnderPower{20};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = true});
	Tick(framesUnderPower);
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = false});

	const double xBeforeTurn = tank->GetPos().x;
	const double yBeforeTurn = tank->GetPos().y;
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
	const std::shared_ptr<Tank> tank = SpawnPlayerAt({.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize});
	_allObjects.emplace_back(std::make_shared<IceTile>(
			ObjRectangle{.x = 0.0, .y = 0.0, .w = _tankSize * 8.0, .h = _tankSize * 8.0}, _events, _uuid, _gameMode));

	constexpr int framesUnderPower{20};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = true});
	Tick(framesUnderPower);
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = false});

	const double yBeforeTap = tank->GetPos().y;
	ASSERT_EQ(tank->GetDirection(), Direction::DOWN);

	_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = true});
	Tick();
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = false});

	Tick(2);

	EXPECT_EQ(tank->GetDirection(), Direction::UP);
	EXPECT_GT(tank->GetPos().y, yBeforeTap);
}
