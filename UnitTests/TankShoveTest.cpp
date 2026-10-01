#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/ObstacleSpawner.h"
#include "components/TankPool.h"
#include "components/events/InputEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/pawns/Tank.h"
#include "enums/Author.h"
#include "enums/Direction.h"
#include "enums/InputChannel.h"
#include "enums/ObstacleType.h"
#include "geometry/Point.h"
#include "gtest/gtest.h"
#include <memory>
#include <vector>

// a tank in the way gives way: each case puts a second tank on the first one's nose, presses the key for
// one frame and reads where both ended up. The pusher is player one, so nothing steers it but the test
class TankShoveTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<TankPool> _tankPool{nullptr};
	std::vector<EventSubscription> _instantSpawnAnimationSubs{};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	EventSubscription _spawnQueueSub{};
	double _deltaTimeOneFrame{1.0 / 60.0};
	double _tankSize{};
	unsigned short _tankHealth{100u};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_obstacleSpawner = std::make_unique<ObstacleSpawner>(_events, _gameConfig);
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_tankPool = std::make_shared<TankPool>(_events, _allObjects, _gameConfig, _bulletPool);
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
		_tankSize = _gameConfig.tankSize;

		_allObjects.reserve(4u);
	}

	std::shared_ptr<Tank> CreatePlayer(const FPoint pos, const Author author, const Direction dir)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _tankSize, .h = _tankSize};

		return TestUtils::CreatePlayer(rect, _tankHealth, author, _allObjects, _events, dir, _tankPool, _gameConfig);
	}

	std::shared_ptr<Tank> CreateBot(const FPoint pos, const Author author, const Direction dir)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _tankSize, .h = _tankSize};

		return TestUtils::CreateBot(rect, _tankHealth, author, _allObjects, _events, dir, _tankPool, _gameConfig);
	}

	std::shared_ptr<BaseObj> SpawnObstacle(const FPoint pos, const ObstacleType type) const
	{
		return TestUtils::SpawnObstacle(_events, _allObjects, pos, type, _gameConfig);
	}

	void SpawnObstacleArea(const ObjRectangle area, const ObstacleType type) const
	{
		TestUtils::SpawnObstacleArea(_events, _allObjects, area, type, _gameConfig);
	}

	void DriveP1Right() const
	{
		_events->EmitEvent(Key(InputChannel::LocalP1), MoveRightEvent{.isPressed = true});
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	}

	//NOTE: two frames - on the first one neither tank has read what the other was asked to do yet
	void DriveTowardsEachOther() const
	{
		_events->EmitEvent(Key(InputChannel::LocalP1), MoveRightEvent{.isPressed = true});
		_events->EmitEvent(Key(InputChannel::LocalP2), MoveLeftEvent{.isPressed = true});
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	}

	[[nodiscard]] double StepOfOneFrame() const { return _gameConfig.tankSpeed * _deltaTimeOneFrame; }
};

// the plain case: the tank ahead is moved along and the one pushing takes the room it made - half a step,
// because that is all that was cleared
TEST_F(TankShoveTest, APushedTankGivesWayAndThePusherFollowsAtHalfSpeed)
{
	const auto pusher{CreatePlayer({.x = 0.0, .y = _tankSize * 2.0}, Author::Player1, Direction::RIGHT)};
	const auto pushed{CreatePlayer({.x = _tankSize, .y = _tankSize * 2.0}, Author::Player2, Direction::UP)};
	const double pusherX{pusher->GetPos().x};
	const double pushedX{pushed->GetPos().x};

	DriveP1Right();

	EXPECT_GT(pushed->GetPos().x, pushedX) << "the tank ahead stood like a wall";
	EXPECT_GT(pusher->GetPos().x, pusherX);
	EXPECT_LT(pusher->GetPos().x - pusherX, StepOfOneFrame()) << "pushing cost the driver nothing";
}

// a tank driving at us is not shoved out of the way, so a meeting head-on holds both where they are
TEST_F(TankShoveTest, AHeadOnMeetingHoldsBothTanks)
{
	const auto pusher{CreatePlayer({.x = 0.0, .y = _tankSize * 2.0}, Author::Player1, Direction::RIGHT)};
	const auto pushed{CreatePlayer({.x = _tankSize, .y = _tankSize * 2.0}, Author::Player2, Direction::LEFT)};

	DriveTowardsEachOther();
	const FPoint pusherPos{pusher->GetPos()};
	const FPoint pushedPos{pushed->GetPos()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(pushed->GetPos(), pushedPos);
	EXPECT_EQ(pusher->GetPos(), pusherPos);
}

// facing is not leaning: a tank standing still gives way whichever way its barrel happens to point
TEST_F(TankShoveTest, ATankMerelyLookingAtUsIsStillPushed)
{
	const auto pusher{CreatePlayer({.x = 0.0, .y = _tankSize * 2.0}, Author::Player1, Direction::RIGHT)};
	const auto pushed{CreatePlayer({.x = _tankSize, .y = _tankSize * 2.0}, Author::Player2, Direction::LEFT)};
	const double pushedX{pushed->GetPos().x};

	DriveP1Right();

	EXPECT_GT(pushed->GetPos().x, pushedX) << "a parked tank held the shove by looking our way";
}

// with its back to a wall the tank ahead has nowhere to give way to, and then it is a wall itself
TEST_F(TankShoveTest, ATankWithItsBackToAWallIsNotPushed)
{
	const auto pusher{CreatePlayer({.x = 0.0, .y = _tankSize * 2.0}, Author::Player1, Direction::RIGHT)};
	const auto pushed{CreatePlayer({.x = _tankSize, .y = _tankSize * 2.0}, Author::Player2, Direction::UP)};
	SpawnObstacleArea(ObjRectangle{.x = _tankSize * 2.0, .y = _tankSize * 2.0,
								   .w = _gameConfig.gridOffset, .h = _tankSize},
					  ObstacleType::Steel);
	const FPoint pusherPos{pusher->GetPos()};
	const FPoint pushedPos{pushed->GetPos()};

	DriveP1Right();

	EXPECT_EQ(pushed->GetPos(), pushedPos);
	EXPECT_EQ(pusher->GetPos(), pusherPos);
}

// an enemy is shoved just the same - the rule is about what stands in the way, not about whose side it is on
TEST_F(TankShoveTest, AnEnemyIsPushedLikeAnyoneElse)
{
	CreatePlayer({.x = 0.0, .y = _tankSize * 2.0}, Author::Player1, Direction::RIGHT);
	const auto enemy{CreateBot({.x = _tankSize, .y = _tankSize * 2.0}, Author::Enemy1, Direction::UP)};
	SpawnObstacle(FPoint{.x = _tankSize, .y = _tankSize}, ObstacleType::Steel);
	const double enemyX{enemy->GetPos().x};

	DriveP1Right();

	EXPECT_GT(enemy->GetPos().x, enemyX);
}

// the chain is asked as a whole: the far tank has room, so the near one gives way by everything asked of it
TEST_F(TankShoveTest, TheWholeChainGivesWayTogether)
{
	const auto near{CreatePlayer({.x = _tankSize, .y = _tankSize * 2.0}, Author::Player1, Direction::UP)};
	CreatePlayer({.x = _tankSize * 2.0 + 1.0, .y = _tankSize * 2.0}, Author::Player2, Direction::UP);
	constexpr double wanted{2.0};

	EXPECT_DOUBLE_EQ(near->ShoveDistance(Direction::RIGHT, wanted, 1), wanted);
}

// and stands as a whole: a wall behind the far tank stops the near one as surely as a wall of its own
TEST_F(TankShoveTest, TheChainStandsWhenItsFarEndCannotMove)
{
	const auto near{CreatePlayer({.x = _tankSize, .y = _tankSize * 2.0}, Author::Player1, Direction::UP)};
	CreatePlayer({.x = _tankSize * 2.0 + 1.0, .y = _tankSize * 2.0}, Author::Player2, Direction::UP);
	SpawnObstacleArea(ObjRectangle{.x = _tankSize * 3.0 + 1.0, .y = _tankSize * 2.0,
								   .w = _gameConfig.gridOffset, .h = _tankSize},
					  ObstacleType::Steel);
	constexpr double wanted{2.0};

	EXPECT_DOUBLE_EQ(near->ShoveDistance(Direction::RIGHT, wanted, 1), 0.0);
}

// on ice the shoved tank glides on after the pusher lets go
TEST_F(TankShoveTest, ATankShovedOnIceSlidesOnAfterThePusherStops)
{
	CreatePlayer({.x = 0.0, .y = _tankSize * 2.0}, Author::Player1, Direction::RIGHT);
	const auto pushed{CreatePlayer({.x = _tankSize, .y = _tankSize * 2.0}, Author::Player2, Direction::UP)};
	SpawnObstacleArea(ObjRectangle{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize * 8.0, .h = _tankSize},
					  ObstacleType::Ice);

	constexpr int framesPushing{10};
	for (int frame{}; frame < framesPushing; ++frame)
	{
		DriveP1Right();
	}
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveRightEvent{.isPressed = false});
	const double pushedX{pushed->GetPos().x};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(pushed->GetPos().x, pushedX) << "the shoved tank stopped dead on ice";
}
