#include "Point.h"
#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/TimingEvents.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/FortressWall.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include <memory>

class BulletTest : public testing::Test// NOLINT(clang-diagnostic-padded)
{
	using buuid = boost::uuids::uuid;

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	GameConfig _gameConfig{"", true};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.f / 60.f};
	BulletCalibre _calibre{.speed = 300.f, .damage = 1u, .damageRadius = 12.0, .tier = 1u, .size{.x = 6.f, .y = 5.f}};
	buuid _uuid{};
	float _gridSize{1};
	unsigned short _bulletHealth{1u};
	GameMode _gameMode{GameMode::OnePlayer};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, &_allObjects);
		_gridSize = static_cast<float>(_gameConfig.windowSize.y) / 50.f;

		_allObjects.reserve(4);
	}

	void TearDown() override
	{
	}
};

// Check that bullet set their position correctly
TEST_F(BulletTest, BulletSetPos)
{
	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "PlayerTeam", &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	const FPoint startPos = bullet->GetPos();

	const auto windowWidth = static_cast<float>(_gameConfig.windowSize.x);
	const auto windowHeight = static_cast<float>(_gameConfig.windowSize.y);
	bullet->SetPos({.x = windowWidth, .y = windowHeight});

	EXPECT_LT(startPos, bullet->GetPos());
}

// Check that bullet set their direction correctly
TEST_F(BulletTest, BulletSetDirection)
{
	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "PlayerTeam", &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	const Direction startDirection = bullet->GetDirection();

	bullet->SetDirection(Direction::UP);

	EXPECT_NE(startDirection, bullet->GetDirection());
	EXPECT_EQ(Direction::UP, bullet->GetDirection());
}

// Check that a bullet can move inside the screen
TEST_F(BulletTest, BulletMoveInsideScreen)
{
	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "PlayerTeam", &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	{
		//success bullet moves down test, try to move inside a screen bullet
		bullet->SetDirection(Direction::DOWN);
		const FPoint bulletStartPos = bullet->GetPos();
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		const FPoint bulletEndPos = bullet->GetPos();
		EXPECT_LT(bulletStartPos.y, bulletEndPos.y);
		EXPECT_EQ(bulletStartPos.x, bulletEndPos.x);
	}
	{
		//success bullet right test, try to move inside a screen bullet
		bullet->SetDirection(Direction::RIGHT);
		const FPoint bulletStartPos = bullet->GetPos();
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		const FPoint bulletEndPos = bullet->GetPos();
		EXPECT_LT(bulletStartPos.x, bulletEndPos.x);
		EXPECT_EQ(bulletStartPos.y, bulletEndPos.y);
	}

	const auto windowWidth = static_cast<float>(_gameConfig.windowSize.x);
	const auto windowHeight = static_cast<float>(_gameConfig.windowSize.y);

	bullet->SetPos({.x = windowWidth - _calibre.size.x, .y = windowHeight - _calibre.size.y});
	{
		//success shot up test, try to create an inside screen bullet
		bullet->SetDirection(Direction::UP);
		const FPoint bulletStartPos = bullet->GetPos();
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		const FPoint bulletEndPos = bullet->GetPos();
		EXPECT_GT(bulletStartPos.y, bulletEndPos.y);
		EXPECT_EQ(bulletStartPos.x, bulletEndPos.x);
	}
	{
		//success move left test, try to move inside a screen bullet
		bullet->SetDirection(Direction::LEFT);
		const FPoint bulletStartPos = bullet->GetPos();
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		const FPoint bulletEndPos = bullet->GetPos();
		EXPECT_GT(bulletStartPos.x, bulletEndPos.x);
		EXPECT_EQ(bulletStartPos.y, bulletEndPos.y);
	}
}

// Check that a bullet can't move outside the screen
TEST_F(BulletTest, BulletMoveOutSideScreen)
{
	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "PlayerTeam", &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	const auto windowWidth = static_cast<float>(_gameConfig.windowSize.x);
	const auto windowHeight = static_cast<float>(_gameConfig.windowSize.y);

	bullet->SetPos({.x = windowWidth - _calibre.size.x, .y = windowHeight - _calibre.size.y});
	{
		//fail bullet move down test, try to move an outside screen bullet
		bullet->SetDirection(Direction::DOWN);
		const FPoint bulletStartPos = bullet->GetPos();
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		EXPECT_EQ(bulletStartPos, bullet->GetPos());
	}
	{
		//fail the bullet move right test, try to move an outside screen bullet
		bullet->SetDirection(Direction::RIGHT);
		const FPoint bulletStartPos = bullet->GetPos();
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		EXPECT_EQ(bulletStartPos, bullet->GetPos());
	}

	bullet->SetPos({.x = 0.f, .y = 0.f});
	{
		//fail bullet move up test, try to move an outside screen bullet
		bullet->SetDirection(Direction::UP);
		const FPoint bulletStartPos = bullet->GetPos();
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		EXPECT_EQ(bulletStartPos, bullet->GetPos());
	}
	{
		//fail the bullet move left test, try to move an outside screen bullet
		bullet->SetDirection(Direction::LEFT);
		const FPoint bulletStartPos = bullet->GetPos();
		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		EXPECT_EQ(bulletStartPos, bullet->GetPos());
	}
}

// Check that a bullet can deal damage to another obstacle
TEST_F(BulletTest, BulletDamageBrickWhenMoveUp)
{
	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.f, .y = 7.f, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "PlayerTeam", &_allObjects,
					_events, _calibre, Direction::UP, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	// spawn BrickWall
	const ObjRectangle rect{.x = 0, .y = 0, .w = _gridSize, .h = _gridSize};
	auto brickWall = std::make_shared<BrickWall>(rect, _events, _uuid, _gameMode);
	_allObjects.emplace_back(brickWall);

	const int brickWallHealth = brickWall->GetHealth();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(brickWallHealth, brickWall->GetHealth());
}

// Check that a bullet can deal damage to another obstacle
TEST_F(BulletTest, BulletDamageBrickWhenMoveLeft)
{
	// spawn Bullet
	const ObjRectangle rectBullet{.x = 7.f, .y = 0.f, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "PlayerTeam", &_allObjects,
					_events, _calibre, Direction::LEFT, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	// spawn BrickWall
	const ObjRectangle rect{.x = 0, .y = 0, .w = _gridSize, .h = _gridSize};
	auto brickWall = std::make_shared<BrickWall>(rect, _events, _uuid, _gameMode);
	_allObjects.emplace_back(brickWall);

	const int brickWallHealth = brickWall->GetHealth();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(brickWallHealth, brickWall->GetHealth());
}

// Check that a bullet can deal damage to another obstacle
TEST_F(BulletTest, BulletDamageBrickWhenMoveDown)
{
	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "PlayerTeam", &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	// spawn BrickWall
	const ObjRectangle rect{.x = 0.f, .y = 6.f, .w = _gridSize, .h = _gridSize};
	auto brickWall = std::make_shared<BrickWall>(rect, _events, _uuid, _gameMode);
	_allObjects.emplace_back(brickWall);

	const int brickWallHealth = brickWall->GetHealth();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(brickWallHealth, brickWall->GetHealth());
}

// Check that a bullet can deal damage to another obstacle
TEST_F(BulletTest, BulletDamageBrickWhenMoveRight)
{
	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "PlayerTeam", &_allObjects,
					_events, _calibre, Direction::RIGHT, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	// spawn BrickWall
	const ObjRectangle rect{.x = 7.f, .y = 0.f, .w = _gridSize, .h = _gridSize};
	auto brickWall = std::make_shared<BrickWall>(rect, _events, _uuid, _gameMode);
	_allObjects.emplace_back(brickWall);

	const int brickWallHealth = brickWall->GetHealth();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(brickWallHealth, brickWall->GetHealth());
}

// Check that a bullet can deal damage to tank
TEST_F(BulletTest, BulletDamageTank)
{
	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "PlayerTeam", &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	// spawn Enemy
	const float gridSize = static_cast<float>(_gameConfig.windowSize.y) / 50.f;
	const float tankSize = gridSize * 3;// for better turns
	constexpr unsigned short tankHealth = 1u;
	constexpr float tankSpeed{142};
	const auto bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _gameConfig);
	const ObjRectangle rectEnemy{.x = 0, .y = _calibre.size.y, .w = tankSize, .h = tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, tankSpeed,
					Direction::UP, _gameMode, bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	EXPECT_EQ(enemyBot->GetHealth(), 1);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(enemyBot->GetHealth(), 0);
}

// Check that a bullet can deal damage to another bullet and self
TEST_F(BulletTest, BulletToBulletDamageEachOther)
{
	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "PlayerTeam", &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	// spawn Bullet2
	const ObjRectangle rectBullet2{.x = 0, .y = _calibre.size.y + 1, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet2 =
			TestUtils::CreateBullet(
					rectBullet2, _bulletHealth, _uuid, "Bullet2", "PlayerTeam", &_allObjects,
					_events, _calibre, Direction::UP, _gameMode, _gameConfig, "Player2");
	_allObjects.emplace_back(bullet2);

	const int bulletHealth = bullet->GetHealth();
	const int bullet2Health = bullet2->GetHealth();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(bulletHealth, bullet->GetHealth());
	EXPECT_GT(bullet2Health, bullet2->GetHealth());
}

// Check that a bullet can't deal damage to an indestructible object
TEST_F(BulletTest, BulletCantDamageSteelWall)
{
	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "PlayerTeam", &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	// spawn SteelWall
	const ObjRectangle rect{.x = 0.f, .y = 6.f, .w = _gridSize, .h = _gridSize};
	auto steelWall = std::make_shared<SteelWall>(rect, _events, _uuid, _gameMode);
	_allObjects.emplace_back(steelWall);

	const int bulletHealth = bullet->GetHealth();
	const int steelWallHealth = steelWall->GetHealth();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(bulletHealth, bullet->GetHealth());
	EXPECT_EQ(steelWallHealth, steelWall->GetHealth());
}

// Check that a bullet can pass through water
TEST_F(BulletTest, BulletCantDamageWater)
{
	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "PlayerTeam", &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	// spawn WaterTile
	const ObjRectangle rect{.x = 0.f, .y = 6.f, .w = _gridSize, .h = _gridSize};
	auto waterTile = std::make_shared<WaterTile>(rect, _events, _uuid, _gameMode);
	_allObjects.emplace_back(waterTile);

	const int bulletHealth = bullet->GetHealth();
	const int waterTileHealth = waterTile->GetHealth();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(bulletHealth, bullet->GetHealth());
	EXPECT_EQ(waterTileHealth, waterTile->GetHealth());
}

// Check that a bullet can deal damage to FortressWall
TEST_F(BulletTest, BulletDamagefortressWall)
{
	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "PlayerTeam", &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	// spawn FortressWall
	constexpr ObjRectangle rect{.x = 0.f, .y = 6.f, .w = 36, .h = 36};
	auto fortressWall = std::make_shared<FortressWall>(rect, _events, &_allObjects, _uuid, _gameMode);
	_allObjects.emplace_back(fortressWall);

	fortressWall->SetHealth(1);
	EXPECT_EQ(fortressWall->GetHealth(), 1);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(fortressWall->GetHealth(), -1);
}

// Check that a bullet deal damage to self when hit something
TEST_F(BulletTest, BulletHaveSelfDamageWhenHit)
{
	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "PlayerTeam", &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	// spawn BrickWall
	constexpr ObjRectangle rect{.x = 0.f, .y = 6.f, .w = 36, .h = 36};
	auto brickWall = std::make_shared<BrickWall>(rect, _events, _uuid, _gameMode);
	_allObjects.emplace_back(brickWall);

	bullet->SetHealth(1);
	EXPECT_EQ(bullet->GetHealth(), 1);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(bullet->GetHealth(), 0);
}
