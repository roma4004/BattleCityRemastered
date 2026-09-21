#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/ObstacleSpawner.h"
#include "components/TankPool.h"
#include "components/events/InputEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/BaseObj.h"
#include "entities/pawns/Tank.h"
#include "enums/Author.h"
#include "enums/Direction.h"
#include "enums/InputChannel.h"
#include "enums/ObstacleType.h"
#include "utils/ColliderUtils.h"
#include "gtest/gtest.h"
#include <memory>
#include <vector>

// a wall with one opening and a tank driving at it from above, started off to the side every time
class TankNudgeTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<TankPool> _tankPool{nullptr};
	std::unique_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	std::vector<EventSubscription> _instantSpawnAnimationSubs{};
	EventSubscription _spawnQueueSub{};
	double _deltaTimeOneFrame{1.0 / 60.0};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_tankPool = std::make_shared<TankPool>(_events, _allObjects, _gameConfig, _bulletPool);
		_obstacleSpawner = std::make_unique<ObstacleSpawner>(_events, _gameConfig);
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
	}

	//NOTE: the wall stands at the tank's own height below it, so the tank has room to pick up speed first
	static constexpr double kWallY{60.0};

	[[nodiscard]] double TankSize() const { return _gameConfig.tankSize; }

	[[nodiscard]] double Cell() const { return _gameConfig.gridOffset; }

	// a row of brick cells from x to x + width, laid cell by cell the way a map does it
	void BuildWall(const double from, const double width) const
	{
		for (double x{from}; x < from + width; x += Cell())
		{
			SpawnObstacle(FPoint{.x = x, .y = kWallY}, ObstacleType::Brick);
		}
	}

	[[nodiscard]] std::shared_ptr<Tank> CreatePlayer(const FPoint pos) const
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};

		return TestUtils::CreatePlayer(rect, _gameConfig.tankHealth, Author::Player1, _allObjects, _events,
									   Direction::DOWN, _tankPool, _gameConfig);
	}

	void Drive(const Direction dir, const int frames) const
	{
		constexpr bool isPressed{true};
		switch (dir)
		{
			case Direction::UP:
				_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = isPressed});
				break;
			case Direction::DOWN:
				_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});
				break;
			case Direction::LEFT:
				_events->EmitEvent(Key(InputChannel::LocalP1), MoveLeftEvent{.isPressed = isPressed});
				break;
			case Direction::RIGHT:
				_events->EmitEvent(Key(InputChannel::LocalP1), MoveRightEvent{.isPressed = isPressed});
				break;
		}

		for (int frame{}; frame < frames; ++frame)
		{
			_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		}
	}

	std::shared_ptr<BaseObj> SpawnObstacle(const FPoint pos, const ObstacleType type) const
	{
		return TestUtils::SpawnObstacle(_events, _allObjects, pos, type, _gameConfig);
	}
};

// the opening has a cell to spare and the tank misses it by less than half itself: it slides across and drives through
TEST_F(TankNudgeTest, ATankMoreThanHalfInTheOpeningIsSteeredIntoIt)
{
	const double gapStart{TankSize() * 2.0};
	BuildWall(0.0, gapStart);
	BuildWall(gapStart + TankSize() + Cell(), TankSize() * 2.0);

	//NOTE: half a cell into the wall - it is the opening it is heading for, not the bricks
	const auto player{CreatePlayer({.x = gapStart + TankSize() / 2.0, .y = 0.0})};

	Drive(Direction::DOWN, 120);

	EXPECT_LE(player->GetRect().Right(), gapStart + TankSize() + Cell()) << "the tank stayed under the wall";
	EXPECT_GT(player->GetRect().y, kWallY) << "the tank never made it through the opening";
}

// the same opening, but the tank is mostly in front of the wall: from there it is going for bricks, not the gap
TEST_F(TankNudgeTest, ATankLessThanHalfInTheOpeningStaysWhereItHitTheWall)
{
	const double gapStart{TankSize() * 2.0};
	BuildWall(0.0, gapStart);
	BuildWall(gapStart + TankSize() + Cell(), TankSize() * 2.0);

	const double startX{gapStart + TankSize()};
	const auto player{CreatePlayer({.x = startX, .y = 0.0})};

	Drive(Direction::DOWN, 120);

	EXPECT_DOUBLE_EQ(player->GetRect().x, startX);
	EXPECT_LT(player->GetRect().y, kWallY);
}

// an opening narrower than the tank: clearing one side puts it into the other, so it stands
TEST_F(TankNudgeTest, AnOpeningNarrowerThanTheTankTakesNoTank)
{
	const double gapStart{TankSize() * 2.0};
	BuildWall(0.0, gapStart);
	BuildWall(gapStart + Cell() * 2.0, TankSize() * 2.0);

	const auto player{CreatePlayer({.x = gapStart, .y = 0.0})};

	Drive(Direction::DOWN, 120);

	EXPECT_DOUBLE_EQ(player->GetRect().x, gapStart);
	EXPECT_LT(player->GetRect().y, kWallY);
}

//NOTE: a corridor is one block wide, the same as a tank - it has to take it edge to edge, nothing to spare
TEST_F(TankNudgeTest, AnOpeningOfExactlyOneTankTakesTheTank)
{
	const double gapStart{TankSize() * 2.0};
	BuildWall(0.0, gapStart);
	BuildWall(gapStart + TankSize(), TankSize() * 2.0);

	const auto player{CreatePlayer({.x = gapStart, .y = 0.0})};

	Drive(Direction::DOWN, 120);

	EXPECT_DOUBLE_EQ(player->GetRect().x, gapStart);
	EXPECT_GT(player->GetRect().y, kWallY);
}

// the point of the blast radius: one shot into a wall has to leave a hole the shooter fits through
TEST_F(TankNudgeTest, AShotIntoAWallLeavesAHoleTheTankFitsThrough)
{
	BuildWall(0.0, TankSize() * 4.0);

	const auto player{CreatePlayer({.x = TankSize(), .y = 0.0})};
	const double startX{player->GetRect().x};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = isPressed});
	Drive(Direction::DOWN, 200);

	EXPECT_DOUBLE_EQ(player->GetRect().x, startX) << "the tank had to steer around what the shot left";
	EXPECT_GT(player->GetRect().y, kWallY) << "the hole was too narrow for the tank that made it";
}

//NOTE: the corridor is cut to the tank's own width - a shift overshooting by a pixel lands in the far wall
TEST_F(TankNudgeTest, AnOpeningExactlyOneTankWideIsEnteredFromHalfATankOff)
{
	const double gapStart{TankSize() * 2.0};
	BuildWall(0.0, gapStart);
	BuildWall(gapStart + TankSize(), TankSize() * 2.0);

	const auto player{CreatePlayer({.x = gapStart - TankSize() / 2.0, .y = 0.0})};

	Drive(Direction::DOWN, 120);

	EXPECT_DOUBLE_EQ(player->GetRect().x, gapStart) << "the tank never lined up with the corridor";
	EXPECT_GT(player->GetRect().y, kWallY) << "the tank stayed under the wall";
}

// The edge of the field is not an opening - a tank nudged past it keeps going, because nothing out
// there ever blocks the way back
TEST_F(TankNudgeTest, ANudgeStopsAtTheFieldEdge)
{
	//NOTE: a third of the tank across - little enough that clearing it sideways looks like a way through
	SpawnObstacle(FPoint{.x = TankSize() - Cell(), .y = TankSize()}, ObstacleType::Brick);

	const auto player{CreatePlayer({.x = 0.0, .y = 0.0})};

	Drive(Direction::DOWN, 60);

	EXPECT_DOUBLE_EQ(player->GetRect().x, 0.0) << "the nudge walked the tank out of the field";
}

// A tank standing against a wall lands a hair inside it as often as a hair short of it, and that hair
// is well under what the collider itself calls a touch - read as "behind me" it opens the wall
TEST_F(TankNudgeTest, ATankAHairInsideAWallDoesNotDriveThroughIt)
{
	//NOTE: two cells, so neither way across clears them - the tank has nowhere to go but stand
	SpawnObstacle(FPoint{.x = 0.0, .y = Cell() * 2.0}, ObstacleType::Brick);
	SpawnObstacle(FPoint{.x = 0.0, .y = Cell() * 3.0}, ObstacleType::Brick);

	const double startX{Cell() - ColliderUtils::kTouchTolerance / 10.0};
	const auto player{CreatePlayer({.x = startX, .y = Cell()})};

	Drive(Direction::LEFT, 60);

	EXPECT_DOUBLE_EQ(player->GetRect().x, startX) << "the tank drove through the wall it was touching";
}
