#include "geometry/Point.h"
#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/TimingEvents.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/FortressWalls.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "gtest/gtest.h"
#include <memory>

class BulletTest : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.0 / 60.0};
	BulletCalibre _calibre{.speed = 300.0, .damage = 1u, .damageRadius = 12.0, .tier = 1u, .size{.x = 6.0, .y = 5.0}};
	Uuid _uuid{};
	double _gridSize{1};
	unsigned short _bulletHealth{1u};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_gridSize = _gameConfig.gridOffset;

		_allObjects.reserve(4);
	}

	void TearDown() override {}
};

TEST_F(BulletTest, BulletSetPos)
{
	const ObjRectangle rectBullet{.x = 0.0, .y = 0.0, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, _allObjects, _events, _calibre,
					Direction::DOWN, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);

	const auto windowWidth = static_cast<double>(_gameConfig.battlefieldSize.x);
	const auto windowHeight = static_cast<double>(_gameConfig.battlefieldSize.y);
	bullet->SetPos({.x = windowWidth, .y = windowHeight});

	EXPECT_EQ(bullet->GetPos(), (FPoint{.x = windowWidth, .y = windowHeight}));
}

TEST_F(BulletTest, BulletSetDirection)
{
	const ObjRectangle rectBullet{.x = 0.0, .y = 0.0, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, _allObjects, _events, _calibre,
					Direction::DOWN, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);

	const Direction startDirection = bullet->GetDirection();

	bullet->SetDirection(Direction::UP);

	EXPECT_NE(startDirection, bullet->GetDirection());
	EXPECT_EQ(Direction::UP, bullet->GetDirection());
}

TEST_F(BulletTest, BulletMoveInsideScreen)
{
	const ObjRectangle rectBullet{.x = 0.0, .y = 0.0, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, _allObjects, _events, _calibre,
					Direction::DOWN, _gameConfig, Author::Player1);
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

	const auto windowWidth = static_cast<double>(_gameConfig.battlefieldSize.x);
	const auto windowHeight = static_cast<double>(_gameConfig.battlefieldSize.y);

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

TEST_F(BulletTest, BulletMoveOutSideScreen)
{
	const ObjRectangle rectBullet{.x = 0.0, .y = 0.0, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, _allObjects, _events, _calibre,
					Direction::DOWN, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);

	const auto windowWidth = static_cast<double>(_gameConfig.battlefieldSize.x);
	const auto windowHeight = static_cast<double>(_gameConfig.battlefieldSize.y);

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

	bullet->SetPos({.x = 0.0, .y = 0.0});
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

TEST_F(BulletTest, BulletDamageBrickWhenMoveUp)
{
	const ObjRectangle rectBullet{.x = 0.0, .y = 7.0, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, _allObjects, _events, _calibre,
					Direction::UP, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);

	const ObjRectangle rect{.x = 0, .y = 0, .w = _gridSize, .h = _gridSize};
	auto brickWall = std::make_shared<BrickWall>(rect, _events, _uuid, _gameConfig);
	_allObjects.emplace_back(brickWall);

	const int brickWallHealth = brickWall->GetHealth();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(brickWallHealth, brickWall->GetHealth());
}

TEST_F(BulletTest, BulletDamageBrickWhenMoveLeft)
{
	const ObjRectangle rectBullet{.x = 7.0, .y = 0.0, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, _allObjects, _events, _calibre,
					Direction::LEFT, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);

	const ObjRectangle rect{.x = 0, .y = 0, .w = _gridSize, .h = _gridSize};
	auto brickWall = std::make_shared<BrickWall>(rect, _events, _uuid, _gameConfig);
	_allObjects.emplace_back(brickWall);

	const int brickWallHealth = brickWall->GetHealth();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(brickWallHealth, brickWall->GetHealth());
}

TEST_F(BulletTest, BulletDamageBrickWhenMoveDown)
{
	const ObjRectangle rectBullet{.x = 0.0, .y = 0.0, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, _allObjects, _events, _calibre,
					Direction::DOWN, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);

	const ObjRectangle rect{.x = 0.0, .y = 6.0, .w = _gridSize, .h = _gridSize};
	auto brickWall = std::make_shared<BrickWall>(rect, _events, _uuid, _gameConfig);
	_allObjects.emplace_back(brickWall);

	const int brickWallHealth = brickWall->GetHealth();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(brickWallHealth, brickWall->GetHealth());
}

TEST_F(BulletTest, BulletDamageBrickWhenMoveRight)
{
	const ObjRectangle rectBullet{.x = 0.0, .y = 0.0, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, _allObjects, _events, _calibre,
					Direction::RIGHT, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);

	const ObjRectangle rect{.x = 7.0, .y = 0.0, .w = _gridSize, .h = _gridSize};
	auto brickWall = std::make_shared<BrickWall>(rect, _events, _uuid, _gameConfig);
	_allObjects.emplace_back(brickWall);

	const int brickWallHealth = brickWall->GetHealth();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(brickWallHealth, brickWall->GetHealth());
}

TEST_F(BulletTest, BulletBlowRadiusIsDirectionSymmetric)
{
	constexpr double mirrorAxis{200.0};
	constexpr double tileSide{2.0};

	auto farTileDamage = [this](const Direction dir)
	{
		const bool isHorizontal = dir == Direction::LEFT || dir == Direction::RIGHT;
		const bool isMirrored = dir == Direction::LEFT || dir == Direction::UP;
		const double bulletLength = isHorizontal ? _calibre.size.x : _calibre.size.y;

		// offset and length are measured along the shot axis, starting at the bullet's back edge
		auto place = [&](const double offset, const double length)
		{
			const double along = isMirrored ? mirrorAxis - offset - length : offset;

			return isHorizontal
					   ? ObjRectangle{.x = along, .y = 0.0, .w = length, .h = tileSide}
					   : ObjRectangle{.x = 0.0, .y = along, .w = tileSide, .h = length};
		};

		_allObjects.clear();
		_allObjects.emplace_back(
				TestUtils::CreateBullet(place(0.0, bulletLength), _bulletHealth, _uuid,
										_allObjects, _events, _calibre, dir, _gameConfig, Author::Player1));
		_allObjects.emplace_back(
				std::make_shared<BrickWall>(place(bulletLength + 1.0, tileSide), _events, _uuid, _gameConfig));

		// just outside the radius measured from the bullet's leading edge, just inside it from the bullet's centre
		auto farTile = std::make_shared<BrickWall>(
				place(bulletLength + 1.0 + _calibre.damageRadius, tileSide), _events, _uuid, _gameConfig);
		_allObjects.emplace_back(farTile);

		const int healthBefore = farTile->GetHealth();

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		return healthBefore - farTile->GetHealth();
	};

	const int damageShotRight = farTileDamage(Direction::RIGHT);

	EXPECT_EQ(damageShotRight, farTileDamage(Direction::LEFT));
	EXPECT_EQ(damageShotRight, farTileDamage(Direction::DOWN));
	EXPECT_EQ(damageShotRight, farTileDamage(Direction::UP));
}

TEST_F(BulletTest, BulletDamageTank)
{
	const ObjRectangle rectBullet{.x = 0.0, .y = 0.0, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, _allObjects, _events, _calibre,
					Direction::DOWN, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);

	const double gridSize = _gameConfig.gridOffset;
	const double tankSize = gridSize * 3;// for better turns
	constexpr unsigned short tankHealth = 1u;
	const auto bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
	const ObjRectangle rectEnemy{.x = 0, .y = _calibre.size.y, .w = tankSize, .h = tankSize};
	std::shared_ptr<Tank> enemyBot =
			TestUtils::CreateBot(
					rectEnemy, tankHealth, _uuid, Author::Enemy1, _allObjects, _events, 1u, Direction::UP, bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	EXPECT_EQ(enemyBot->GetHealth(), 1);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(enemyBot->GetHealth(), 0);
}

TEST_F(BulletTest, BulletToBulletDamageEachOther)
{
	const ObjRectangle rectBullet{.x = 0.0, .y = 0.0, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, _allObjects, _events, _calibre,
					Direction::DOWN, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);

	const ObjRectangle rectBullet2{.x = 0, .y = _calibre.size.y + 1, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet2 =
			TestUtils::CreateBullet(
					rectBullet2, _bulletHealth, _uuid, _allObjects, _events, _calibre,
					Direction::UP, _gameConfig, Author::Player2);
	_allObjects.emplace_back(bullet2);

	const int bulletHealth = bullet->GetHealth();
	const int bullet2Health = bullet2->GetHealth();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(bulletHealth, bullet->GetHealth());
	EXPECT_GT(bullet2Health, bullet2->GetHealth());
}

TEST_F(BulletTest, BulletCantDamageSteelWall)
{
	const ObjRectangle rectBullet{.x = 0.0, .y = 0.0, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, _allObjects, _events, _calibre,
					Direction::DOWN, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);

	const ObjRectangle rect{.x = 0.0, .y = 6.0, .w = _gridSize, .h = _gridSize};
	auto steelWall = std::make_shared<SteelWall>(rect, _events, _uuid, _gameConfig);
	_allObjects.emplace_back(steelWall);

	const int bulletHealth = bullet->GetHealth();
	const int steelWallHealth = steelWall->GetHealth();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_GT(bulletHealth, bullet->GetHealth());
	EXPECT_EQ(steelWallHealth, steelWall->GetHealth());
}

TEST_F(BulletTest, BulletCantDamageWater)
{
	const ObjRectangle rectBullet{.x = 0.0, .y = 0.0, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, _allObjects, _events, _calibre,
					Direction::DOWN, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);

	const ObjRectangle rect{.x = 0.0, .y = 6.0, .w = _gridSize, .h = _gridSize};
	auto waterTile = std::make_shared<WaterTile>(rect, _events, _uuid, _gameConfig);
	_allObjects.emplace_back(waterTile);

	const int bulletHealth = bullet->GetHealth();
	const int waterTileHealth = waterTile->GetHealth();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(bulletHealth, bullet->GetHealth());
	EXPECT_EQ(waterTileHealth, waterTile->GetHealth());
}

TEST_F(BulletTest, BulletDamagefortressWall)
{
	const ObjRectangle rectBullet{.x = 0.0, .y = 0.0, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, _allObjects, _events, _calibre,
					Direction::DOWN, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);

	constexpr ObjRectangle rect{.x = 0.0, .y = 6.0, .w = 36, .h = 36};
	auto fortressWall = std::make_shared<FortressBrickWall>(rect, _events, _uuid, _gameConfig);
	_allObjects.emplace_back(fortressWall);

	fortressWall->SetHealth(1);
	EXPECT_EQ(fortressWall->GetHealth(), 1);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(fortressWall->GetIsAlive());
}

TEST_F(BulletTest, BulletHaveSelfDamageWhenHit)
{
	const ObjRectangle rectBullet{.x = 0.0, .y = 0.0, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, _allObjects, _events, _calibre,
					Direction::DOWN, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);

	constexpr ObjRectangle rect{.x = 0.0, .y = 6.0, .w = 36, .h = 36};
	auto brickWall = std::make_shared<BrickWall>(rect, _events, _uuid, _gameConfig);
	_allObjects.emplace_back(brickWall);

	bullet->SetHealth(1);
	EXPECT_EQ(bullet->GetHealth(), 1);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(bullet->GetHealth(), 0);
}
