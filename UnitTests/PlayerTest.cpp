#include "TestUtils.h"
#include "components/ObstacleSpawner.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/TankPool.h"
#include "components/EventSystem.h"
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
#include "gtest/gtest.h"
#include <memory>

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
									   const Direction dir = Direction::UP)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};
		auto player{TestUtils::CreatePlayer(rect, _tankHealth, author, _allObjects, _events, dir, _tankPool,
											_gameConfig)};

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
};

// room above, so the press carries the tank up and leaves the other axis alone
TEST_F(PlayerTest, TankMoveInSideScreenUp)
{
	const auto windowHeight{static_cast<double>(_gameConfig.battlefieldSize.y)};
	const auto player{CreatePlayer({.x = 0.0, .y = windowHeight - _tankSize})};

	const FPoint startPos{player->GetPos()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const FPoint endPos{player->GetPos()};
	EXPECT_EQ(startPos.x, endPos.x);
	EXPECT_GT(startPos.y, endPos.y);
}

// the same step to the left, from the far side of the field
TEST_F(PlayerTest, TankMoveInSideScreenLeft)
{
	const auto windowWidth{static_cast<double>(_gameConfig.battlefieldSize.x)};
	const auto player{CreatePlayer({.x = windowWidth - _tankSize, .y = 0.0})};

	const FPoint startPos{player->GetPos()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveLeftEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const FPoint endPos{player->GetPos()};
	EXPECT_EQ(startPos.y, endPos.y);
	EXPECT_GT(startPos.x, endPos.x);
}

// and down
TEST_F(PlayerTest, TankMoveInSideScreenDown)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0})};

	const FPoint startPos{player->GetPos()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const FPoint endPos{player->GetPos()};
	EXPECT_EQ(startPos.x, endPos.x);
	EXPECT_LT(startPos.y, endPos.y);
}

// and right - four directions, one rule
TEST_F(PlayerTest, TankMoveInSideScreenRight)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0})};

	const FPoint startPos{player->GetPos()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveRightEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const FPoint endPos{player->GetPos()};
	EXPECT_EQ(startPos.y, endPos.y);
	EXPECT_LT(startPos.x, endPos.x);
}

// standing on the top edge: the press turns the hull and the tank stays put
TEST_F(PlayerTest, TankMoveOutSideScreenUp)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0})};

	const FPoint startPos{player->GetPos()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// the same against the left edge
TEST_F(PlayerTest, TankMoveOutSideScreenLeft)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0})};

	const FPoint startPos{player->GetPos()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveLeftEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// against the bottom one
TEST_F(PlayerTest, TankMoveOutSideScreenDown)
{
	const auto windowWidth{static_cast<double>(_gameConfig.battlefieldSize.x)};
	const auto windowHeight{static_cast<double>(_gameConfig.battlefieldSize.y)};
	const auto player{CreatePlayer({.x = windowWidth - _tankSize, .y = windowHeight - _tankSize})};

	const FPoint startPos{player->GetPos()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// and against the right - the field ends the drive in every direction
TEST_F(PlayerTest, TankMoveOutSideScreenRight)
{
	const auto windowWidth{static_cast<double>(_gameConfig.battlefieldSize.x)};
	const auto windowHeight{static_cast<double>(_gameConfig.battlefieldSize.y)};
	const auto player{CreatePlayer({.x = windowWidth - _tankSize, .y = windowHeight - _tankSize})};

	const FPoint startPos{player->GetPos()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveRightEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// a placement bypassing the keys, the way a respawn or the wire puts a tank down
TEST_F(PlayerTest, TankSetPos)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0})};

	const auto windowWidth{static_cast<double>(_gameConfig.battlefieldSize.x)};
	const auto windowHeight{static_cast<double>(_gameConfig.battlefieldSize.y)};
	player->SetPos({.x = windowWidth, .y = windowHeight});

	EXPECT_EQ(player->GetPos(), (FPoint{.x = windowWidth, .y = windowHeight}));
}

// and the same for the direction it faces
TEST_F(PlayerTest, TankSetDirection)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0}, Author::Player1, Direction::LEFT)};

	const Direction startDirection{player->GetDirection()};

	player->SetDirection(Direction::RIGHT);

	EXPECT_NE(startDirection, player->GetDirection());
	EXPECT_EQ(Direction::RIGHT, player->GetDirection());
}

// firing is not driving: the shot leaves and the tank stays where it stood
TEST_F(PlayerTest, TankDontMoveWhenShotUp)
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

// the same facing left
TEST_F(PlayerTest, TankDontMoveWhenShotLeft)
{
	const auto windowWidth{static_cast<double>(_gameConfig.battlefieldSize.x)};
	const auto windowHeight{static_cast<double>(_gameConfig.battlefieldSize.y)};
	const auto player{CreatePlayer({.x = windowWidth / 2.0, .y = windowHeight / 2.0}, Author::Player1,
								   Direction::LEFT)};

	const FPoint startPos{player->GetPos()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// down
TEST_F(PlayerTest, TankDontMoveWhenShotDown)
{
	const auto windowWidth{static_cast<double>(_gameConfig.battlefieldSize.x)};
	const auto windowHeight{static_cast<double>(_gameConfig.battlefieldSize.y)};
	const auto player{CreatePlayer({.x = windowWidth / 2.0, .y = windowHeight / 2.0}, Author::Player1,
								   Direction::DOWN)};

	const FPoint startPos{player->GetPos()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// and right
TEST_F(PlayerTest, TankDontMoveWhenShotRight)
{
	const auto windowWidth{static_cast<double>(_gameConfig.battlefieldSize.x)};
	const auto windowHeight{static_cast<double>(_gameConfig.battlefieldSize.y)};
	const auto player{CreatePlayer({.x = windowWidth / 2.0, .y = windowHeight / 2.0}, Author::Player1,
								   Direction::RIGHT)};

	const FPoint startPos{player->GetPos()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// with the muzzle pointed into the field the shot is born and the world grows by one
TEST_F(PlayerTest, TankShotInSideScreenDown)
{
	CreatePlayer({.x = 0.0, .y = 0.0}, Author::Player1, Direction::DOWN);

	const size_t size{_allObjects.size()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(size, _allObjects.size());
}

// the same facing right
TEST_F(PlayerTest, TankShotInSideScreenRight)
{
	CreatePlayer({.x = 0.0, .y = 0.0}, Author::Player1, Direction::RIGHT);

	const size_t size{_allObjects.size()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveRightEvent{.isPressed = isPressed});
	_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(size, _allObjects.size());
}

// up
TEST_F(PlayerTest, TankShotInSideScreenUp)
{
	const auto windowWidth{static_cast<double>(_gameConfig.battlefieldSize.x)};
	const auto windowHeight{static_cast<double>(_gameConfig.battlefieldSize.y)};
	CreatePlayer({.x = windowWidth - _tankSize, .y = windowHeight - _tankSize}, Author::Player1, Direction::RIGHT);

	const size_t size{_allObjects.size()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = isPressed});
	_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(size, _allObjects.size());
}

// and left
TEST_F(PlayerTest, TankShotInSideScreenLeft)
{
	const auto windowWidth{static_cast<double>(_gameConfig.battlefieldSize.x)};
	const auto windowHeight{static_cast<double>(_gameConfig.battlefieldSize.y)};
	CreatePlayer({.x = windowWidth - _tankSize, .y = windowHeight - _tankSize}, Author::Player1, Direction::RIGHT);

	const size_t size{_allObjects.size()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveLeftEvent{.isPressed = isPressed});
	_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(size, _allObjects.size());
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

// two seats driving into each other - neither is pushed, both stay where they were
TEST_F(PlayerTest, TankCantPassThroughTank)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0})};
	const auto player2{CreatePlayer({.x = 0, .y = _tankSize + 1}, Author::Player2)};

	const FPoint playerStartPos{player->GetPos()};
	const FPoint player2StartPos{player2->GetPos()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveLeftEvent{.isPressed = isPressed});
	_events->EmitEvent(Key(InputChannel::LocalP2), MoveUpEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(playerStartPos, player->GetPos());
	EXPECT_EQ(player2StartPos, player2->GetPos());
}

// a brick wall stops the drive as well
TEST_F(PlayerTest, TankCantPassThroughBrickWall)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0})};

	SpawnObstacle(ObjRectangle{.x = 0.0, .y = _tankSize + 1, .w = _tankSize, .h = _gridSize}, ObstacleType::Brick);

	const FPoint startPos{player->GetPos()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// so does steel
TEST_F(PlayerTest, TankCantPassThroughSteelWall)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0})};

	SpawnObstacle(ObjRectangle{.x = 0.0, .y = _tankSize + 1, .w = _tankSize, .h = _gridSize}, ObstacleType::Steel);

	const FPoint startPos{player->GetPos()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// and water, which a bullet flies over but a tank cannot ford
TEST_F(PlayerTest, TankCantPassThroughWater)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0})};

	SpawnObstacle(ObjRectangle{.x = 0.0, .y = _tankSize + 1, .w = _tankSize, .h = _gridSize}, ObstacleType::Water);

	const FPoint startPos{player->GetPos()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// and the eagle's own wall
TEST_F(PlayerTest, TankCantPassThroughfortressWall)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0})};

	SpawnObstacle(ObjRectangle{.x = 0.0, .y = _tankSize + 1, .w = _tankSize, .h = _gridSize}, ObstacleType::Fortress);

	const FPoint startPos{player->GetPos()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// firing while driving forward: the bullet outruns the hull instead of blowing up on it
TEST_F(PlayerTest, ShotWhileMovingDoesNotBlowUpOnOwnTank)
{
	const auto windowWidth{static_cast<double>(_gameConfig.battlefieldSize.x)};
	const auto windowHeight{static_cast<double>(_gameConfig.battlefieldSize.y)};
	const auto player{CreatePlayer({.x = windowWidth / 2.0, .y = windowHeight / 2.0}, Author::Player1,
								   Direction::LEFT)};

	const int startHealth{player->GetHealth()};

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

	const int startHealth{player->GetHealth()};

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), FireEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(player->GetHealth(), startHealth) << "own blast did not reach the shooter";
}

// The driver holds its own keyed subscriptions, so it has to go quiet with the tank
TEST_F(PlayerTest, APlayerTankIgnoresKeysPressedWhileDeactivated)
{
	const auto player{CreatePlayer({.x = _tankSize * 2.0, .y = _tankSize * 2.0})};

	player->Deactivate();
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = true});
	player->Activate();

	const FPoint before{player->GetPos()};
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(player->GetPos(), before);

	//the control: the very same press drives it once the tank is there to hear it
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = true});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_NE(player->GetPos(), before);
}
