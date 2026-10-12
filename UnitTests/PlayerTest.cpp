#include "TestUtils.h"
#include "components/ObstacleSpawner.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/TankPool.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/TimingEvents.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "components/managers/GameStateManager.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/FortressWalls.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "enums/InputChannel.h"
#include "geometry/Point.h"
#include "utils/DirectionUtils.h"
#include "gtest/gtest.h"
#include <array>
#include <cstddef>
#include <memory>
#include <ranges>
#include <utility>

// the player seat end to end: a key is pressed on the P1 channel, one tick runs, and the tank is asked where it stands
class PlayerTest : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<TankPool> _tankPool{nullptr};
	std::shared_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<GameStateManager> _stateManager{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<RespawnManager> _respawnManager{nullptr};
	std::vector<EventSubscription> _instantSpawnAnimationSubs{};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.0 / 60.0};
	Uuid _uuid{};
	double _tankSize{};
	double _gridSize{};
	unsigned short _tankHealth{100u};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_obstacleSpawner = std::make_unique<ObstacleSpawner>(_events, _gameConfig);
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_tankPool = std::make_shared<TankPool>(_events, _allObjects, _gameConfig, _bulletPool);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, _allObjects, _gameConfig);
		_stateManager = std::make_shared<GameStateManager>(_events);
		TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, _gameConfig.gameMode, _respawnManager,
								 _tankSpawner);
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
		_gridSize = _gameConfig.gridOffset;
		_tankSize = _gameConfig.tankSize;

		_allObjects.reserve(4u);
	}

	void TearDown() override {}

	std::shared_ptr<Tank> CreatePlayer(const FPoint pos, const Author author = Author::Player1,
									   const Direction dir = Direction::UP, const unsigned short tier = 1u)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};
		auto player{TestUtils::CreatePlayer(rect, _tankHealth, author, _allObjects, _events, dir, _tankPool,
											_gameConfig, tier)};

		return player;
	}

	void SpawnObstacleArea(const ObjRectangle area, const ObstacleType type) const
	{
		TestUtils::SpawnObstacleArea(_events, _allObjects, area, type, _gameConfig);
	}

	std::shared_ptr<BaseObj> SpawnObstacle(const ObjRectangle rect, const ObstacleType type) const
	{
		return TestUtils::SpawnObstacle(_events, _allObjects, rect, type);
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
};

// a tank facing its key with room ahead is carried that way, in each of the four, and the other axis is left alone
TEST_F(PlayerTest, TankMoveInsideScreen)
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
TEST_F(PlayerTest, TankMoveOutsideScreen)
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
TEST_F(PlayerTest, TankDontMoveWhenShot)
{
	const auto windowWidth{static_cast<double>(_gameConfig.battlefieldSize.x)};
	const auto windowHeight{static_cast<double>(_gameConfig.battlefieldSize.y)};
	const auto player{CreatePlayer({.x = windowWidth / 2.0, .y = windowHeight / 2.0})};

	const FPoint startPos{player->GetPos()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// with the muzzle pointed into the field the shot is born and the world grows by one, whichever way it points
TEST_F(PlayerTest, TankShotInsideScreen)
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
TEST_F(PlayerTest, TankShotOutSideScreen)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0})};

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
TEST_F(PlayerTest, TankCantPassThroughAWall)
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
TEST_F(PlayerTest, ShotWhileMovingDoesNotBlowUpOnOwnTank)
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
TEST_F(PlayerTest, PointBlankShotDamagesTheShooter)
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
TEST_F(PlayerTest, TheTopTierFiresAVolleyAsWideAsTheHull)
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

// the lead shell goes off at the edge of the field, and the one behind it flies on through that blast
TEST_F(PlayerTest, AShellOfTheVolleySurvivesTheBlastOfTheOneAhead)
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

// point blank only the first row goes off - a row born past the wall would be a shot through it
TEST_F(PlayerTest, APointBlankVolleyFiresOnlyItsFirstRow)
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
TEST_F(PlayerTest, ATapOnANewDirectionOnlyTurnsTheHull)
{
	const auto player{CreatePlayer({.x = _tankSize * 2.0, .y = _tankSize * 2.0})};
	const FPoint before{player->GetPos()};

	_events->EmitEvent(Key(InputChannel::LocalP1), MoveRightEvent{.isPressed = true});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(player->GetDirection(), Direction::RIGHT);
	EXPECT_EQ(player->GetPos(), before);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(player->GetPos().x, before.x);
}
