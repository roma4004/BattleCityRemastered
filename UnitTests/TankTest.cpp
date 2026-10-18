#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/ObstacleSpawner.h"
#include "components/TankPool.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/BaseObj.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/BushTile.h"
#include "entities/obstacles/FortressWalls.h"
#include "entities/obstacles/IceTile.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/pawns/Tank.h"
#include "enums/Author.h"
#include "enums/Direction.h"
#include "enums/InputChannel.h"
#include "enums/ObstacleType.h"
#include "geometry/ObjRectangle.h"
#include "geometry/Point.h"
#include "utils/ColliderUtils.h"
#include "utils/DirectionUtils.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <array>
#include <cstddef>
#include <memory>
#include <ranges>
#include <utility>
#include <vector>

// what every tank does, a player's or a bot's: driving, walls, openings, pushing, terrain and firing
class TankTest : public testing::Test
{
protected:
	//NOTE: the wall stands at the tank's own height below it, so the tank has room to pick up speed first
	static constexpr double kWallY{60.0};

	std::shared_ptr<EventSystem> _events{std::make_shared<EventSystem>()};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	std::unique_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<TankPool> _tankPool{nullptr};
	std::vector<EventSubscription> _instantSpawnAnimationSubs{};
	EventSubscription _spawnQueueSub{};
	double _deltaTimeOneFrame{1.0 / 60.0};
	double _tankSize{};
	double _gridSize{};

	void SetUp() override
	{
		_obstacleSpawner = std::make_unique<ObstacleSpawner>(_events, _gameConfig);
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_tankPool = std::make_shared<TankPool>(_events, _allObjects, _gameConfig, _bulletPool);
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
		_tankSize = _gameConfig.tankSize;
		_gridSize = _gameConfig.gridOffset;

		_allObjects.reserve(4u);
	}

	std::shared_ptr<Tank> CreatePlayer(const FPoint pos, const Author author, const Direction dir,
									   const unsigned short tier = 1u) const
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _tankSize, .h = _tankSize};

		return TestUtils::CreatePlayer(rect, _gameConfig.tankHealth, author, _allObjects, _events, dir, _tankPool,
									   _gameConfig, tier);
	}

	std::shared_ptr<Tank> CreateBot(const FPoint pos, const Author author, const Direction dir) const
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _tankSize, .h = _tankSize};

		return TestUtils::CreateBot(rect, _gameConfig.tankHealth, author, _allObjects, _events, dir, _tankPool,
									_gameConfig);
	}

	std::shared_ptr<BaseObj> SpawnObstacle(const FPoint pos, const ObstacleType type) const
	{
		return TestUtils::SpawnObstacle(_events, _allObjects, pos, type, _gameConfig);
	}

	std::shared_ptr<BaseObj> SpawnObstacle(const ObjRectangle rect, const ObstacleType type) const
	{
		return TestUtils::SpawnObstacle(_events, _allObjects, rect, type);
	}

	void SpawnObstacleArea(const ObjRectangle area, const ObstacleType type) const
	{
		TestUtils::SpawnObstacleArea(_events, _allObjects, area, type, _gameConfig);
	}

	// a row of brick cells from x to x + width, laid cell by cell the way a map does it
	void BuildWall(const double from, const double width) const
	{
		for (double x{from}; x < from + width; x += _gridSize)
		{
			SpawnObstacle(FPoint{.x = x, .y = kWallY}, ObstacleType::Brick);
		}
	}

	void Press(const Direction dir) const
	{
		switch (dir)
		{
			case Direction::UP:
				_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = true});
				break;
			case Direction::DOWN:
				_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = true});
				break;
			case Direction::LEFT:
				_events->EmitEvent(Key(InputChannel::LocalP1), MoveLeftEvent{.isPressed = true});
				break;
			case Direction::RIGHT:
				_events->EmitEvent(Key(InputChannel::LocalP1), MoveRightEvent{.isPressed = true});
				break;
		}
	}

	void Tick(const int frames) const
	{
		for (int frame{}; frame < frames; ++frame)
		{
			_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		}
	}

	void Drive(const Direction dir, const int frames) const
	{
		Press(dir);
		Tick(frames);
	}

	[[nodiscard]] double StepOfOneFrame() const { return _gameConfig.tankSpeed * _deltaTimeOneFrame; }
};

// a tank facing its key with room ahead is carried that way, in each of the four, and the other axis is left alone
TEST_F(TankTest, DrivesAlongItsKey)
{
	const auto width{static_cast<double>(_gameConfig.battlefieldSize.x)};
	const auto height{static_cast<double>(_gameConfig.battlefieldSize.y)};
	const std::array starts{std::pair{FPoint{.x = 0.0, .y = height - _tankSize}, Direction::UP},
							std::pair{FPoint{.x = width - _tankSize, .y = 0.0}, Direction::LEFT},
							std::pair{FPoint{.x = 0.0, .y = 0.0}, Direction::DOWN},
							std::pair{FPoint{.x = 0.0, .y = 0.0}, Direction::RIGHT}};
	for (const auto& [start, dir]: starts)
	{
		SCOPED_TRACE(static_cast<int>(dir));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		const auto player{CreatePlayer(start, Author::Player1, dir)};

		Press(dir);
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		const auto [unitX, unitY]{DirectionUtils::Unit(dir)};
		const double alongX{player->GetPos().x - start.x};
		const double alongY{player->GetPos().y - start.y};
		EXPECT_GT(alongX * unitX + alongY * unitY, 0.0) << "the tank did not move along the key";
		EXPECT_EQ(unitX == 0.0 ? alongX : alongY, 0.0) << "the tank drifted across the key";
	}
}

// standing against an edge and facing it: the press leaves the tank where it is - the field ends the drive
TEST_F(TankTest, StopsAtTheFieldEdge)
{
	const FPoint farCorner{.x = static_cast<double>(_gameConfig.battlefieldSize.x) - _tankSize,
						   .y = static_cast<double>(_gameConfig.battlefieldSize.y) - _tankSize};
	const std::array starts{std::pair{FPoint{}, Direction::UP}, std::pair{FPoint{}, Direction::LEFT},
							std::pair{farCorner, Direction::DOWN}, std::pair{farCorner, Direction::RIGHT}};
	for (const auto& [start, dir]: starts)
	{
		SCOPED_TRACE(static_cast<int>(dir));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		const auto player{CreatePlayer(start, Author::Player1, dir)};

		Press(dir);
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(start, player->GetPos());
	}
}

// firing is not driving: the shot leaves and the tank stays where it stood
TEST_F(TankTest, StandsWhileFiring)
{
	const auto windowWidth{static_cast<double>(_gameConfig.battlefieldSize.x)};
	const auto windowHeight{static_cast<double>(_gameConfig.battlefieldSize.y)};
	const auto player{CreatePlayer({.x = windowWidth / 2.0, .y = windowHeight / 2.0}, Author::Player1, Direction::UP)};

	const FPoint startPos{player->GetPos()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// with the muzzle pointed into the field the shot is born and the world grows by one, whichever way it points
TEST_F(TankTest, FiresIntoTheField)
{
	const FPoint farCorner{.x = static_cast<double>(_gameConfig.battlefieldSize.x) - _tankSize,
						   .y = static_cast<double>(_gameConfig.battlefieldSize.y) - _tankSize};
	const std::array starts{std::pair{FPoint{}, Direction::DOWN}, std::pair{FPoint{}, Direction::RIGHT},
							std::pair{farCorner, Direction::UP}, std::pair{farCorner, Direction::LEFT}};
	for (const auto& [start, dir]: starts)
	{
		SCOPED_TRACE(static_cast<int>(dir));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		CreatePlayer(start, Author::Player1, dir);
		const std::size_t size{_allObjects.size()};

		_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = true});
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_LT(size, _allObjects.size());
	}
}

// in each corner, facing out: there is no room for the bullet, so nothing is spawned
TEST_F(TankTest, HoldsFireOutOfTheField)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0}, Author::Player1, Direction::UP)};

	constexpr bool isPressed{true};
	{
		const size_t size{_allObjects.size()};

		_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = isPressed});
		_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = isPressed});
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(size, _allObjects.size());
	}
	{
		const size_t size{_allObjects.size()};

		_events->EmitEvent(Key(InputChannel::LocalP1), MoveLeftEvent{.isPressed = isPressed});
		_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = isPressed});
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(size, _allObjects.size());
	}

	player->SetPos({.x = static_cast<double>(_gameConfig.battlefieldSize.x) - _tankSize,
					.y = static_cast<double>(_gameConfig.battlefieldSize.y) - _tankSize});
	{
		const size_t size{_allObjects.size()};

		_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});
		_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = isPressed});
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(size, _allObjects.size());
	}
	{
		const size_t size{_allObjects.size()};

		_events->EmitEvent(Key(InputChannel::LocalP1), MoveRightEvent{.isPressed = isPressed});
		_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = isPressed});
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(size, _allObjects.size());
	}
}

// a wall flush ahead stops the drive - brick, steel and the eagle's own wall alike
TEST_F(TankTest, WallStopsIt)
{
	constexpr std::array kWalls{ObstacleType::Brick, ObstacleType::Steel, ObstacleType::Fortress};
	for (const ObstacleType type: kWalls)
	{
		SCOPED_TRACE(static_cast<int>(type));
		_events->EmitEvent(GameResetEvent{});
		_allObjects.clear();
		const auto player{CreatePlayer({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};
		SpawnObstacle(ObjRectangle{.x = 0.0, .y = _tankSize, .w = _tankSize, .h = _gridSize}, type);

		const FPoint startPos{player->GetPos()};

		_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = true});
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(startPos, player->GetPos());
	}
}

// firing while driving forward: the bullet outruns the hull instead of blowing up on it
TEST_F(TankTest, ShotOutrunsTheMovingHull)
{
	const auto windowWidth{static_cast<double>(_gameConfig.battlefieldSize.x)};
	const auto windowHeight{static_cast<double>(_gameConfig.battlefieldSize.y)};
	const auto player{CreatePlayer({.x = windowWidth / 2.0, .y = windowHeight / 2.0}, Author::Player1,
								   Direction::LEFT)};

	const unsigned int startHealth{player->GetHealth()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveLeftEvent{.isPressed = isPressed});
	_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	ASSERT_EQ(_allObjects.size(), 2u) << "no bullet was spawned";
	const std::shared_ptr<BaseObj> bullet{_allObjects.back()};

	for (int frame = 0; frame < 5; ++frame)
	{
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	}

	EXPECT_EQ(_allObjects.size(), 2u) << "the tank fired a second time mid-flight";
	EXPECT_TRUE(bullet->GetIsAlive()) << "bullet died on its own tank";
	EXPECT_EQ(player->GetHealth(), startHealth) << "tank damaged by its own bullet";
}

// a wall a muzzle's length away, on the other hand, puts the blast back onto the shooter
TEST_F(TankTest, PointBlankBlastHitsTheShooter)
{
	const auto windowWidth{static_cast<double>(_gameConfig.battlefieldSize.x)};
	const auto windowHeight{static_cast<double>(_gameConfig.battlefieldSize.y)};
	const auto player{CreatePlayer({.x = windowWidth / 2.0, .y = windowHeight / 2.0}, Author::Player1,
								   Direction::LEFT)};

	const ObjRectangle rectWall{.x = player->GetRect().x - _gridSize - 12.0,
								.y = player->GetRect().y,
								.w = _gridSize,
								.h = _tankSize};
	SpawnObstacleArea(rectWall, ObstacleType::Steel);

	const unsigned int startHealth{player->GetHealth()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(player->GetHealth(), startHealth) << "own blast did not reach the shooter";
}

// the top tier fires a volley, and none of it wider than the hull - where the tank gets through, its volley does too
TEST_F(TankTest, TopTierVolleyIsHullWide)
{
	const auto windowWidth{static_cast<double>(_gameConfig.battlefieldSize.x)};
	const auto windowHeight{static_cast<double>(_gameConfig.battlefieldSize.y)};
	const auto player{CreatePlayer({.x = windowWidth / 2.0, .y = windowHeight / 2.0}, Author::Player1,
								   Direction::UP, 7u)};
	const std::size_t before{_allObjects.size()};

	_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = true});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	ASSERT_GT(_allObjects.size(), before + 1u) << "no volley was fired";
	const ObjRectangle hull{player->GetRect()};
	for (const std::shared_ptr<BaseObj>& shell: _allObjects | std::views::drop(before))
	{
		EXPECT_GE(shell->GetRect().x, hull.x);
		EXPECT_LE(shell->GetRect().Right(), hull.Right());
	}
}

// the lead shell deals its damage at the field edge, and the one behind it flies on through that blast
TEST_F(TankTest, VolleyShellSurvivesTheBlastAhead)
{
	CreatePlayer({.x = _tankSize * 2.0, .y = _tankSize * 2.0}, Author::Player1, Direction::UP, 4u);
	const std::size_t before{_allObjects.size()};

	_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = true});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = false});

	ASSERT_EQ(_allObjects.size(), before + 2u);
	const std::shared_ptr<BaseObj> behind{_allObjects[before]};
	const std::shared_ptr<BaseObj> ahead{_allObjects[before + 1u]};
	ASSERT_LT(ahead->GetRect().y, behind->GetRect().y);

	for (int frame{}; frame < 60 && ahead->GetIsAlive(); ++frame)
	{
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	}

	ASSERT_FALSE(ahead->GetIsAlive()) << "the lead shell never reached the edge";
	EXPECT_TRUE(behind->GetIsAlive()) << "the blast of the lead shell took the one behind it";
}

// point blank only the first row fires - a row spawned past the wall would shoot through it
TEST_F(TankTest, PointBlankVolleyFiresFirstRowOnly)
{
	const auto windowWidth{static_cast<double>(_gameConfig.battlefieldSize.x)};
	const auto windowHeight{static_cast<double>(_gameConfig.battlefieldSize.y)};
	const auto player{CreatePlayer({.x = windowWidth / 2.0, .y = windowHeight / 2.0}, Author::Player1,
								   Direction::LEFT, 4u)};
	SpawnObstacleArea({.x = player->GetRect().x - _gridSize - 12.0,
					   .y = player->GetRect().y,
					   .w = _gridSize,
					   .h = _tankSize}, ObstacleType::Brick);
	const std::size_t before{_allObjects.size()};

	_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = true});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_allObjects.size(), before + 1u);
}

// A tap on a new direction spends its frame on the turn, and the hull sets off on the next one
TEST_F(TankTest, TapOnlyTurns)
{
	const auto player{CreatePlayer({.x = _tankSize * 2.0, .y = _tankSize * 2.0}, Author::Player1, Direction::UP)};
	const FPoint before{player->GetPos()};

	_events->EmitEvent(Key(InputChannel::LocalP1), MoveRightEvent{.isPressed = true});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(player->GetDirection(), Direction::RIGHT);
	EXPECT_EQ(player->GetPos(), before);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(player->GetPos().x, before.x);
}

// the plain case: the tank ahead is moved, and the pusher takes the room it made - half a step, all that cleared
TEST_F(TankTest, PushedTankGivesWayAtHalfSpeed)
{
	const auto pusher{CreatePlayer({.x = 0.0, .y = _tankSize * 2.0}, Author::Player1, Direction::RIGHT)};
	const auto pushed{CreatePlayer({.x = _tankSize, .y = _tankSize * 2.0}, Author::Player2, Direction::UP)};
	const double pusherX{pusher->GetPos().x};
	const double pushedX{pushed->GetPos().x};

	_events->EmitEvent(Key(InputChannel::LocalP1), MoveRightEvent{.isPressed = true});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(pushed->GetPos().x, pushedX) << "the tank ahead stood like a wall";
	EXPECT_GT(pusher->GetPos().x, pusherX);
	EXPECT_LT(pusher->GetPos().x - pusherX, StepOfOneFrame()) << "pushing cost the driver nothing";
}

// a tank driving at us is not shoved out of the way, so a meeting head-on holds both where they are
TEST_F(TankTest, HeadOnMeetingHoldsBoth)
{
	const auto pusher{CreatePlayer({.x = 0.0, .y = _tankSize * 2.0}, Author::Player1, Direction::RIGHT)};
	const auto pushed{CreatePlayer({.x = _tankSize, .y = _tankSize * 2.0}, Author::Player2, Direction::LEFT)};

	_events->EmitEvent(Key(InputChannel::LocalP1), MoveRightEvent{.isPressed = true});
	_events->EmitEvent(Key(InputChannel::LocalP2), MoveLeftEvent{.isPressed = true});
	//NOTE: two frames - on the first one neither tank has read what the other was asked to do yet
	Tick(2);
	const FPoint pusherPos{pusher->GetPos()};
	const FPoint pushedPos{pushed->GetPos()};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(pushed->GetPos(), pushedPos);
	EXPECT_EQ(pusher->GetPos(), pusherPos);
}

// facing is not leaning: a tank standing still gives way whichever way its barrel happens to point
TEST_F(TankTest, FacingDoesNotResistAPush)
{
	const auto pusher{CreatePlayer({.x = 0.0, .y = _tankSize * 2.0}, Author::Player1, Direction::RIGHT)};
	const auto pushed{CreatePlayer({.x = _tankSize, .y = _tankSize * 2.0}, Author::Player2, Direction::LEFT)};
	const double pushedX{pushed->GetPos().x};

	_events->EmitEvent(Key(InputChannel::LocalP1), MoveRightEvent{.isPressed = true});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(pushed->GetPos().x, pushedX) << "a parked tank held the shove by looking our way";
}

// with its back to a wall the tank ahead has nowhere to give way to, and then it is a wall itself
TEST_F(TankTest, TankAgainstAWallIsNotPushed)
{
	const auto pusher{CreatePlayer({.x = 0.0, .y = _tankSize * 2.0}, Author::Player1, Direction::RIGHT)};
	const auto pushed{CreatePlayer({.x = _tankSize, .y = _tankSize * 2.0}, Author::Player2, Direction::UP)};
	SpawnObstacleArea(ObjRectangle{.x = _tankSize * 2.0, .y = _tankSize * 2.0,
								   .w = _gameConfig.gridOffset, .h = _tankSize},
					  ObstacleType::Steel);
	const FPoint pusherPos{pusher->GetPos()};
	const FPoint pushedPos{pushed->GetPos()};

	_events->EmitEvent(Key(InputChannel::LocalP1), MoveRightEvent{.isPressed = true});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(pushed->GetPos(), pushedPos);
	EXPECT_EQ(pusher->GetPos(), pusherPos);
}

// the chain is asked as a whole: the far tank has room, so the near one gives way and takes it along
TEST_F(TankTest, ChainGivesWayTogether)
{
	CreatePlayer({.x = 0.0, .y = _tankSize * 2.0}, Author::Player1, Direction::RIGHT);
	const auto near{CreatePlayer({.x = _tankSize, .y = _tankSize * 2.0}, Author::Player2, Direction::UP)};
	const auto far{CreatePlayer({.x = _tankSize * 2.0, .y = _tankSize * 2.0}, Author::Player3, Direction::UP)};
	const double nearX{near->GetPos().x};
	const double farX{far->GetPos().x};

	_events->EmitEvent(Key(InputChannel::LocalP1), MoveRightEvent{.isPressed = true});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(near->GetPos().x, nearX) << "the near tank stood like a wall";
	EXPECT_GT(far->GetPos().x, farX) << "the far tank was left in the near one's way";
}

// and stands as a whole: a wall behind the far tank stops the near one as surely as a wall of its own
TEST_F(TankTest, ChainStandsWhenItsEndIsBlocked)
{
	const auto pusher{CreatePlayer({.x = 0.0, .y = _tankSize * 2.0}, Author::Player1, Direction::RIGHT)};
	const auto near{CreatePlayer({.x = _tankSize, .y = _tankSize * 2.0}, Author::Player2, Direction::UP)};
	CreatePlayer({.x = _tankSize * 2.0, .y = _tankSize * 2.0}, Author::Player3, Direction::UP);
	SpawnObstacleArea(ObjRectangle{.x = _tankSize * 3.0, .y = _tankSize * 2.0,
								   .w = _gameConfig.gridOffset, .h = _tankSize},
					  ObstacleType::Steel);
	const FPoint pusherPos{pusher->GetPos()};
	const FPoint nearPos{near->GetPos()};

	_events->EmitEvent(Key(InputChannel::LocalP1), MoveRightEvent{.isPressed = true});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(near->GetPos(), nearPos);
	EXPECT_EQ(pusher->GetPos(), pusherPos);
}

// on ice the shoved tank glides on after the pusher lets go
TEST_F(TankTest, PushedTankSlidesOnIce)
{
	CreatePlayer({.x = 0.0, .y = _tankSize * 2.0}, Author::Player1, Direction::RIGHT);
	const auto pushed{CreatePlayer({.x = _tankSize, .y = _tankSize * 2.0}, Author::Player2, Direction::UP)};
	SpawnObstacleArea(ObjRectangle{.x = 0.0, .y = _tankSize * 2.0, .w = _tankSize * 8.0, .h = _tankSize},
					  ObstacleType::Ice);

	constexpr int framesPushing{10};
	for (int frame{}; frame < framesPushing; ++frame)
	{
		_events->EmitEvent(Key(InputChannel::LocalP1), MoveRightEvent{.isPressed = true});
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	}
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveRightEvent{.isPressed = false});
	const double pushedX{pushed->GetPos().x};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(pushed->GetPos().x, pushedX) << "the shoved tank stopped dead on ice";
}

// the opening has a cell to spare and the tank misses it by less than half itself: it slides across and drives through
TEST_F(TankTest, MoreThanHalfInAGapIsSteeredIn)
{
	const double gapStart{_tankSize * 2.0};
	BuildWall(0.0, gapStart);
	BuildWall(gapStart + _tankSize + _gridSize, _tankSize * 2.0);

	//NOTE: half a cell into the wall - it is the opening it is heading for, not the bricks
	const auto player{CreatePlayer({.x = gapStart + _tankSize / 2.0, .y = 0.0}, Author::Player1, Direction::DOWN)};

	Drive(Direction::DOWN, 120);

	EXPECT_LE(player->GetRect().Right(), gapStart + _tankSize + _gridSize) << "the tank stayed under the wall";
	EXPECT_GT(player->GetRect().y, kWallY) << "the tank never made it through the opening";
}

// the same opening, but the tank is mostly in front of the wall: from there it is going for bricks, not the gap
TEST_F(TankTest, LessThanHalfInAGapStays)
{
	const double gapStart{_tankSize * 2.0};
	BuildWall(0.0, gapStart);
	BuildWall(gapStart + _tankSize + _gridSize, _tankSize * 2.0);

	const double startX{gapStart + _tankSize};
	const auto player{CreatePlayer({.x = startX, .y = 0.0}, Author::Player1, Direction::DOWN)};

	Drive(Direction::DOWN, 120);

	EXPECT_DOUBLE_EQ(player->GetRect().x, startX);
	EXPECT_LT(player->GetRect().y, kWallY);
}

// an opening narrower than the tank: clearing one side puts it into the other, so it stands
TEST_F(TankTest, GapNarrowerThanTheTankTakesNone)
{
	const double gapStart{_tankSize * 2.0};
	BuildWall(0.0, gapStart);
	BuildWall(gapStart + _gridSize * 2.0, _tankSize * 2.0);

	const auto player{CreatePlayer({.x = gapStart, .y = 0.0}, Author::Player1, Direction::DOWN)};

	Drive(Direction::DOWN, 120);

	EXPECT_DOUBLE_EQ(player->GetRect().x, gapStart);
	EXPECT_LT(player->GetRect().y, kWallY);
}

//NOTE: a corridor is one block wide, the same as a tank - it has to take it edge to edge, nothing to spare
TEST_F(TankTest, TankWideGapTakesTheTank)
{
	const double gapStart{_tankSize * 2.0};
	BuildWall(0.0, gapStart);
	BuildWall(gapStart + _tankSize, _tankSize * 2.0);

	const auto player{CreatePlayer({.x = gapStart, .y = 0.0}, Author::Player1, Direction::DOWN)};

	Drive(Direction::DOWN, 120);

	EXPECT_DOUBLE_EQ(player->GetRect().x, gapStart);
	EXPECT_GT(player->GetRect().y, kWallY);
}

// a star grows the tank, and standing on a wall it grows away from it - grown into the wall it would be stuck there
TEST_F(TankTest, StarGrowsAwayFromTheWall)
{
	BuildWall(0.0, _tankSize * 3.0);
	const auto player{CreatePlayer({.x = _tankSize, .y = kWallY - _tankSize}, Author::Player1, Direction::DOWN)};

	_events->EmitEvent(Key(Author::Player1), BonusStarPickupEvent{});

	const ObjRectangle hull{player->GetRect()};
	const auto isInside = [&player, &hull](const std::shared_ptr<BaseObj>& object)
	{
		return object != player && ColliderUtils::IsCollide(hull, object->GetRect());
	};
	EXPECT_GT(hull.w, _tankSize);
	EXPECT_TRUE(std::ranges::none_of(_allObjects, isInside));
}

// a star picked up in an opening cut to the tank's width - grown into the walls it could not drive out of them
TEST_F(TankTest, StarInATightGapGrowsOnceOut)
{
	const double gapStart{_tankSize * 2.0};
	BuildWall(0.0, gapStart);
	BuildWall(gapStart + _tankSize, _tankSize * 2.0);
	const auto player{CreatePlayer({.x = gapStart, .y = kWallY - _gridSize}, Author::Player1, Direction::DOWN)};

	_events->EmitEvent(Key(Author::Player1), BonusStarPickupEvent{});

	EXPECT_GT(player->GetTier(), 1u) << "the model has to change at once, only the size waits";
	EXPECT_DOUBLE_EQ(player->GetRect().w, _tankSize) << "the tank grew into the walls on both sides";

	Drive(Direction::DOWN, 120);

	const ObjRectangle hull{player->GetRect()};
	const auto isInside = [&player, &hull](const std::shared_ptr<BaseObj>& object)
	{
		return object != player && ColliderUtils::IsCollide(hull, object->GetRect());
	};
	EXPECT_GT(hull.w, _tankSize) << "out in the open the tank still waits for room";
	EXPECT_TRUE(std::ranges::none_of(_allObjects, isInside));
}

// the point of the blast radius: one shot into a wall has to leave a hole the shooter fits through
TEST_F(TankTest, ShotLeavesAHoleTheTankFits)
{
	BuildWall(0.0, _tankSize * 4.0);

	const auto player{CreatePlayer({.x = _tankSize, .y = 0.0}, Author::Player1, Direction::DOWN)};
	const double startX{player->GetRect().x};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = isPressed});
	Drive(Direction::DOWN, 200);

	EXPECT_DOUBLE_EQ(player->GetRect().x, startX) << "the tank had to steer around what the shot left";
	EXPECT_GT(player->GetRect().y, kWallY) << "the hole was too narrow for the tank that made it";
}

//NOTE: the corridor is cut to the tank's own width - a shift overshooting by a pixel lands in the far wall
TEST_F(TankTest, TankWideGapIsEnteredFromHalfOff)
{
	const double gapStart{_tankSize * 2.0};
	BuildWall(0.0, gapStart);
	BuildWall(gapStart + _tankSize, _tankSize * 2.0);

	const auto player{CreatePlayer({.x = gapStart - _tankSize / 2.0, .y = 0.0}, Author::Player1, Direction::DOWN)};

	Drive(Direction::DOWN, 120);

	EXPECT_DOUBLE_EQ(player->GetRect().x, gapStart) << "the tank never lined up with the corridor";
	EXPECT_GT(player->GetRect().y, kWallY) << "the tank stayed under the wall";
}

// the field edge is no opening - a tank nudged past it would keep going, nothing out there blocks the way back
TEST_F(TankTest, NudgeStopsAtTheFieldEdge)
{
	//NOTE: a third of the tank across - little enough that clearing it sideways looks like a way through
	SpawnObstacle(FPoint{.x = _tankSize - _gridSize, .y = _tankSize}, ObstacleType::Brick);

	const auto player{CreatePlayer({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};

	Drive(Direction::DOWN, 60);

	EXPECT_DOUBLE_EQ(player->GetRect().x, 0.0) << "the nudge walked the tank out of the field";
}

// a tank against a wall lands a hair inside it as often as short of it - read as "behind me" it opens the wall
TEST_F(TankTest, HairInsideAWallDoesNotPassIt)
{
	//NOTE: two cells, so neither way across clears them - the tank has nowhere to go but stand
	SpawnObstacle(FPoint{.x = 0.0, .y = _gridSize * 2.0}, ObstacleType::Brick);
	SpawnObstacle(FPoint{.x = 0.0, .y = _gridSize * 3.0}, ObstacleType::Brick);

	const double startX{_gridSize - ColliderUtils::kTouchTolerance / 10.0};
	const auto player{CreatePlayer({.x = startX, .y = _gridSize}, Author::Player1, Direction::DOWN)};

	Drive(Direction::LEFT, 60);

	EXPECT_DOUBLE_EQ(player->GetRect().x, startX) << "the tank drove through the wall it was touching";
}

// in a bush a tank shows no health bar, which would give it away - on open ground and once the bush is gone, it is back
TEST_F(TankTest, HealthBarHidesInABush)
{
	const std::shared_ptr<Tank> tank{CreatePlayer({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	ASSERT_TRUE(tank->ShownHealth().has_value()) << "no bar on open ground";

	SpawnObstacle(FPoint{.x = 0.0, .y = 0.0}, ObstacleType::Bush);
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	ASSERT_FALSE(tank->ShownHealth().has_value()) << "the bar gave the hidden tank away";

	_allObjects.pop_back();
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_TRUE(tank->ShownHealth().has_value());
}

//NOTE: on ice Move feeds velocity instead of moving - ApplyMoveVelocity spends it later
TEST_F(TankTest, SlidesOnIceAfterTheKeyIsReleased)
{
	const std::shared_ptr<Tank> tank{CreatePlayer({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};
	constexpr int framesUnderPower{20};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = true});
	Tick(framesUnderPower);
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = false});
	const double yOnGround{tank->GetPos().y};
	ASSERT_GT(yOnGround, 0.0);
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	ASSERT_DOUBLE_EQ(tank->GetPos().y, yOnGround) << "on plain ground the tank is not where the key left it";

	SpawnObstacleArea(ObjRectangle{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize * 8.0}, ObstacleType::Ice);
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = true});
	Tick(framesUnderPower);
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = false});
	const double yOnRelease{tank->GetPos().y};

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(tank->GetPos().y, yOnRelease);
}

//NOTE: momentum is per direction, so a turn mid-drift spends both at once
TEST_F(TankTest, TurnWhileDriftingSlidesDiagonally)
{
	const std::shared_ptr<Tank> tank{CreatePlayer({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};
	SpawnObstacleArea(ObjRectangle{.x = 0.0, .y = 0.0, .w = _tankSize * 8.0, .h = _tankSize * 8.0}, ObstacleType::Ice);

	constexpr int framesUnderPower{20};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = true});
	Tick(framesUnderPower);
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = false});

	const double xBeforeTurn{tank->GetPos().x};
	const double yBeforeTurn{tank->GetPos().y};
	ASSERT_DOUBLE_EQ(xBeforeTurn, 0.0);

	_events->EmitEvent(Key(InputChannel::LocalP1), MoveRightEvent{.isPressed = true});
	//NOTE: the first frame only turns the hull
	Tick(2);

	EXPECT_EQ(tank->GetDirection(), Direction::RIGHT);
	EXPECT_GT(tank->GetPos().x, xBeforeTurn);
	EXPECT_GT(tank->GetPos().y, yBeforeTurn);
}

//NOTE: a tap only turns the sprite - one frame of reverse momentum is spent at once
TEST_F(TankTest, TurningAroundKeepsTheDrift)
{
	const std::shared_ptr<Tank> tank{CreatePlayer({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN)};
	SpawnObstacleArea(ObjRectangle{.x = 0.0, .y = 0.0, .w = _tankSize * 8.0, .h = _tankSize * 8.0}, ObstacleType::Ice);

	constexpr int framesUnderPower{20};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = true});
	Tick(framesUnderPower);
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = false});

	const double yBeforeTap{tank->GetPos().y};
	ASSERT_EQ(tank->GetDirection(), Direction::DOWN);

	_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = true});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = false});

	Tick(2);

	EXPECT_EQ(tank->GetDirection(), Direction::UP);
	EXPECT_GT(tank->GetPos().y, yBeforeTap);
}
