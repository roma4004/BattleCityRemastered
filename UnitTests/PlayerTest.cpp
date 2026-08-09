#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/InputEvents.h"
#include "components/events/TimingEvents.h"
#include "components/TankSpawner.h"
#include "components/managers/DelayedSpawnManager.h"
#include "components/managers/RespawnManager.h"
#include "components/managers/GameStateManager.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/FortressWall.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/pawns/Player.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include <memory>
#include <boost/uuid/random_generator.hpp>

class PlayerTest : public testing::Test// NOLINT(clang-diagnostic-padded)
{
	using buuid = boost::uuids::uuid;

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<GameStateManager> _stateManager{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<RespawnManager> _respawnManager{nullptr};
	std::shared_ptr<DelayedSpawnManager> _spawnDelayManager{nullptr};
	GameConfig _gameConfig{"", true};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.f / 60.f};
	buuid _uuid{};// boost::uuids::uuid is internally 8-byte aligned (uses a uint64_t for alignment)
	float _tankSize{};
	float _tankSpeed{142};
	float _gridSize{};
	unsigned short _tankHealth{100u};
	GameMode _gameMode{GameMode::OnePlayer};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, &_allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _gameConfig);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, &_allObjects, _gameConfig);
		_stateManager = std::make_shared<GameStateManager>(_events);
		_respawnManager = std::make_shared<RespawnManager>(_events);
		_tankSpawner = std::make_shared<TankSpawner>(_gameConfig, &_allObjects, _events);
		_spawnDelayManager = std::make_shared<DelayedSpawnManager>(_events);
		_gridSize = static_cast<float>(_gameConfig.windowSize.y) / 50.f;
		_tankSize = _gridSize * 3.f;// for better turns

		_allObjects.reserve(4u);
	}

	void TearDown() override
	{
		_events->RemoveListener<AddToSpawnQueueEvent>("TestSpawnQueue");
	}
};

// Check that tank can move inside the screen
TEST_F(PlayerTest, TankMoveInSideScreenUp)
{
	const auto windowHeight = static_cast<float>(_gameConfig.windowSize.y);
	const ObjRectangle rectPlayer{.x = 0.f, .y = windowHeight - _tankSize, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const FPoint startPos = player->GetPos();

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(std::string{"P1"}), MoveUpEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const FPoint endPos = player->GetPos();
	EXPECT_NE(startPos.y, endPos.y);
	EXPECT_EQ(startPos.x, endPos.x);
	EXPECT_GT(startPos.y, endPos.y);
}

// Check that tank can move inside the screen
TEST_F(PlayerTest, TankMoveInSideScreenLeft)
{
	const auto windowWidth = static_cast<float>(_gameConfig.windowSize.x);
	const ObjRectangle rectPlayer{.x = windowWidth - _tankSize, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const FPoint startPos = player->GetPos();

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(std::string{"P1"}), MoveLeftEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const FPoint endPos = player->GetPos();
	EXPECT_NE(startPos.x, endPos.x);
	EXPECT_EQ(startPos.y, endPos.y);
	EXPECT_GT(startPos.x, endPos.x);
}

// Check that tank can move inside the screen
TEST_F(PlayerTest, TankMoveInSideScreenDown)
{
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const FPoint startPos = player->GetPos();

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(std::string{"P1"}), MoveDownEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const FPoint endPos = player->GetPos();
	EXPECT_NE(startPos.y, endPos.y);
	EXPECT_EQ(startPos.x, endPos.x);
	EXPECT_LT(startPos.y, endPos.y);
}

// Check that tank can move inside the screen
TEST_F(PlayerTest, TankMoveInSideScreenRight)
{
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const FPoint startPos = player->GetPos();

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(std::string{"P1"}), MoveRightEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	const FPoint endPos = player->GetPos();
	EXPECT_NE(startPos.x, endPos.x);
	EXPECT_EQ(startPos.y, endPos.y);
	EXPECT_LT(startPos.x, endPos.x);
}

// Check that tank cannot move out of screen
TEST_F(PlayerTest, TankMoveOutSideScreenUp)
{
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const FPoint startPos = player->GetPos();

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(std::string{"P1"}), MoveUpEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// Check that tank cannot move out of screen
TEST_F(PlayerTest, TankMoveOutSideScreenLeft)
{
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const FPoint startPos = player->GetPos();

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(std::string{"P1"}), MoveLeftEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// Check that tank cannot move out of screen
TEST_F(PlayerTest, TankMoveOutSideScreenDown)
{
	const auto windowWidth = static_cast<float>(_gameConfig.windowSize.x);
	const auto windowHeight = static_cast<float>(_gameConfig.windowSize.y);
	const ObjRectangle rectPlayer{.x = windowWidth - _tankSize,
								  .y = windowHeight - _tankSize,
								  .w = _tankSize,
								  .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const FPoint startPos = player->GetPos();

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(std::string{"P1"}), MoveDownEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// Check that tank cannot move out of screen
TEST_F(PlayerTest, TankMoveOutSideScreenRight)
{
	const auto windowWidth = static_cast<float>(_gameConfig.windowSize.x);
	const auto windowHeight = static_cast<float>(_gameConfig.windowSize.y);
	const ObjRectangle rectPlayer{.x = windowWidth - _tankSize,
								  .y = windowHeight - _tankSize,
								  .w = _tankSize,
								  .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const FPoint startPos = player->GetPos();

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(std::string{"P1"}), MoveRightEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// Check that tank set their position correctly
TEST_F(PlayerTest, TankSetPos)
{
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const FPoint startPos = player->GetPos();

	const auto windowWidth = static_cast<float>(_gameConfig.windowSize.x);
	const auto windowHeight = static_cast<float>(_gameConfig.windowSize.y);
	player->SetPos({.x = windowWidth, .y = windowHeight});

	EXPECT_LT(startPos, player->GetPos());
}

// Check that tank set their direction correctly
TEST_F(PlayerTest, TankSetDirection)
{
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::LEFT, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const Direction startDirection = player->GetDirection();

	player->SetDirection(Direction::RIGHT);

	EXPECT_NE(startDirection, player->GetDirection());
	EXPECT_EQ(Direction::RIGHT, player->GetDirection());
}

// Check that tank don't move when shooting
TEST_F(PlayerTest, TankDontMoveWhenShotUp)
{
	const auto windowWidth = static_cast<float>(_gameConfig.windowSize.x);
	const auto windowHeight = static_cast<float>(_gameConfig.windowSize.y);
	const ObjRectangle rectPlayer{.x = windowWidth / 2.f, .y = windowHeight / 2.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const FPoint startPos = player->GetPos();

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(std::string{"P1"}), FireEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// Check that tank don't move when shooting
TEST_F(PlayerTest, TankDontMoveWhenShotLeft)
{
	const auto windowWidth = static_cast<float>(_gameConfig.windowSize.x);
	const auto windowHeight = static_cast<float>(_gameConfig.windowSize.y);
	const ObjRectangle rectPlayer{.x = windowWidth / 2.f, .y = windowHeight / 2.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::LEFT, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const FPoint startPos = player->GetPos();

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(std::string{"P1"}), FireEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// Check that tank don't move when shooting
TEST_F(PlayerTest, TankDontMoveWhenShotDown)
{
	const auto windowWidth = static_cast<float>(_gameConfig.windowSize.x);
	const auto windowHeight = static_cast<float>(_gameConfig.windowSize.y);
	const ObjRectangle rectPlayer{.x = windowWidth / 2.f, .y = windowHeight / 2.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const FPoint startPos = player->GetPos();

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(std::string{"P1"}), FireEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// Check that tank doesn't move when shooting
TEST_F(PlayerTest, TankDontMoveWhenShotRight)
{
	const auto windowWidth = static_cast<float>(_gameConfig.windowSize.x);
	const auto windowHeight = static_cast<float>(_gameConfig.windowSize.y);
	const ObjRectangle rectPlayer{.x = windowWidth / 2.f, .y = windowHeight / 2.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::RIGHT, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const FPoint startPos = player->GetPos();

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(std::string{"P1"}), FireEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// Check that tank can shoot inside the screen
TEST_F(PlayerTest, TankShotInSideScreenDown)
{
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	//success shot down test, try to create an inside screen bullet
	const size_t size = _allObjects.size();

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(std::string{"P1"}), FireEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(size, _allObjects.size());
}

// Check that tank can shoot inside the screen
TEST_F(PlayerTest, TankShotInSideScreenRight)
{
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::RIGHT, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	//success shot right test, try to create an inside screen bullet
	const size_t size = _allObjects.size();

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(std::string{"P1"}), MoveRightEvent{.isPressed = isPressed});
	_events->EmitEvent(Key(std::string{"P1"}), FireEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(size, _allObjects.size());
}

// Check that tank can shoot inside the screen
TEST_F(PlayerTest, TankShotInSideScreenUp)
{
	const auto windowWidth = static_cast<float>(_gameConfig.windowSize.x);
	const auto windowHeight = static_cast<float>(_gameConfig.windowSize.y);
	const ObjRectangle rectPlayer{.x = windowWidth - _tankSize,
								  .y = windowHeight - _tankSize,
								  .w = _tankSize,
								  .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::RIGHT, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	//success shot up test, try to create an inside screen bullet
	const size_t size = _allObjects.size();

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(std::string{"P1"}), MoveUpEvent{.isPressed = isPressed});
	_events->EmitEvent(Key(std::string{"P1"}), FireEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(size, _allObjects.size());
}

// Check that tank can shoot inside the screen
TEST_F(PlayerTest, TankShotInSideScreenLeft)
{
	const auto windowWidth = static_cast<float>(_gameConfig.windowSize.x);
	const auto windowHeight = static_cast<float>(_gameConfig.windowSize.y);
	const ObjRectangle rectPlayer{.x = windowWidth - _tankSize,
								  .y = windowHeight - _tankSize,
								  .w = _tankSize,
								  .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::RIGHT, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	//success shot left test, try to create an inside screen bullet
	const size_t size = _allObjects.size();

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(std::string{"P1"}), MoveLeftEvent{.isPressed = isPressed});
	_events->EmitEvent(Key(std::string{"P1"}), FireEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_LT(size, _allObjects.size());
}

// Check that tank can't shoot outside the screen
TEST_F(PlayerTest, TankShotOutSideScreen)
{
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	constexpr bool isPressed{true};
	{
		//fail the shot up test, try to create an outside screen bullet
		const size_t size = _allObjects.size();

		_events->EmitEvent(Key(std::string{"P1"}), MoveUpEvent{.isPressed = isPressed});
		_events->EmitEvent(Key(std::string{"P1"}), FireEvent{.isPressed = isPressed});
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(size, _allObjects.size());
	}
	{
		//fail shot left test, try to create an outside screen bullet
		const size_t size = _allObjects.size();

		_events->EmitEvent(Key(std::string{"P1"}), MoveLeftEvent{.isPressed = isPressed});
		_events->EmitEvent(Key(std::string{"P1"}), FireEvent{.isPressed = isPressed});
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(size, _allObjects.size());
	}

	player->SetPos({.x = static_cast<float>(_gameConfig.windowSize.x) - _tankSize,
					.y = static_cast<float>(_gameConfig.windowSize.y) - _tankSize});
	{
		//fail the shot down test, try to create an outside screen bullet
		const size_t size = _allObjects.size();

		_events->EmitEvent(Key(std::string{"P1"}), MoveDownEvent{.isPressed = isPressed});
		_events->EmitEvent(Key(std::string{"P1"}), FireEvent{.isPressed = isPressed});
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(size, _allObjects.size());
	}
	{
		//fail the shot right test, try to create an outside screen bullet
		const size_t size = _allObjects.size();

		_events->EmitEvent(Key(std::string{"P1"}), MoveRightEvent{.isPressed = isPressed});
		_events->EmitEvent(Key(std::string{"P1"}), FireEvent{.isPressed = isPressed});
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_EQ(size, _allObjects.size());
	}
}

// Check that tank can't move through tank
TEST_F(PlayerTest, TankCantPassThroughTank)
{
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const ObjRectangle rectPlayer2 = {.x = 0, .y = _tankSize + 1, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player2 =
			TestUtils::CreateTank<Player>(
					rectPlayer2, _tankHealth, _uuid, "Player2", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player2);

	//both players should failure, because they face each other blocking move each other
	const FPoint playerStartPos = player->GetPos();
	const FPoint player2StartPos = player2->GetPos();

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(std::string{"P1"}), MoveLeftEvent{.isPressed = isPressed});
	_events->EmitEvent(Key(std::string{"P2"}), MoveUpEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(playerStartPos, player->GetPos());
	EXPECT_EQ(player2StartPos, player2->GetPos());
}

// Check that tank can't move through brickWall
TEST_F(PlayerTest, TankCantPassThroughBrickWall)
{
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	_allObjects.emplace_back(
			std::make_shared<BrickWall>(
					ObjRectangle{.x = 0.f, .y = _tankSize + 1, .w = _gridSize, .h = _gridSize}, _events, _uuid,
					_gameMode));

	//moveDown player should failure, because below we have a brickWall obstacle
	const FPoint startPos = player->GetPos();

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(std::string{"P1"}), MoveDownEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// Check that tank cant move through steelWall
TEST_F(PlayerTest, TankCantPassThroughSteelWall)
{
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	_allObjects.emplace_back(
			std::make_shared<SteelWall>(
					ObjRectangle{.x = 0.f, .y = _tankSize + 1, .w = _gridSize, .h = _gridSize}, _events, _uuid,
					_gameMode));

	//moveDown player should failure, because below we have a steelWall obstacle
	const FPoint startPos = player->GetPos();

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(std::string{"P1"}), MoveDownEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// Check that tank cant move through water
TEST_F(PlayerTest, TankCantPassThroughWater)
{
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	auto waterTile = std::make_shared<WaterTile>(
			ObjRectangle{.x = 0.f, .y = _tankSize + 1, .w = _gridSize, .h = _gridSize},
			_events, _uuid, _gameMode);
	_allObjects.emplace_back(waterTile);

	//moveDown player should failure, because below we have a water obstacle
	const FPoint startPos = player->GetPos();

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(std::string{"P1"}), MoveDownEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}

// Check that tank can't move through fortressWall
TEST_F(PlayerTest, TankCantPassThroughfortressWall)
{
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	auto fortressWall = std::make_shared<FortressWall>(
			ObjRectangle{.x = 0.f, .y = _tankSize + 1, .w = _gridSize, .h = _gridSize}, _events,
			&_allObjects, _uuid, _gameMode);
	_allObjects.emplace_back(fortressWall);

	//moveDown player should failure, because below we have a fortressWall obstacle
	const FPoint startPos = player->GetPos();

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(std::string{"P1"}), MoveDownEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(startPos, player->GetPos());
}
