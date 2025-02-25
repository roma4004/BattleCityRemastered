#include "../headers/EventSystem.h"
#include "../headers/GameMode.h"
#include "../headers/Point.h"
#include "../headers/obstacles/BrickWall.h"
#include "../headers/obstacles/FortressWall.h"
#include "../headers/obstacles/SteelWall.h"
#include "../headers/obstacles/WaterTile.h"
#include "../headers/pawns/Bullet.h"
#include "../headers/pawns/Enemy.h"

#include "gtest/gtest.h"

#include <memory>

class BulletTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<int[]> _windowBuffer{nullptr};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	UPoint _windowSize{.x = 800, .y = 600};
	FPoint _bulletSize;
	float _bulletSpeed{300.f};
	float _gridSize{1};
	float _deltaTimeOneFrame{1.f / 60.f};

	void SetUp() override
	{
		_gridSize = static_cast<float>(_windowSize.y) / 50.f;
		_events = std::make_shared<EventSystem>();
		_bulletSize = FPoint{.x = 6.f, .y = 5.f};
		ObjRectangle bulletRect{.x = 0.f, .y = 0.f, .w = _bulletSize.x, .h = _bulletSize.y};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		std::string name = "Player";
		std::string fraction = "PlayerTeam";
		_allObjects.reserve(4);
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, DOWN, _bulletSpeed, &_allObjects,
		                                                  _events, name, fraction, 0, false));
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

//TODO: extract to base object tests like set pos
// Check that tank set their position correctly
TEST_F(BulletTest, BulletSetPos)
{
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		bullet->SetPos({});
		const FPoint startPos = bullet->GetPos();

		bullet->SetPos({.x = static_cast<float>(_windowSize.x), .y = static_cast<float>(_windowSize.y)});

		EXPECT_LT(startPos, bullet->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that bullet can move inside of screen
TEST_F(BulletTest, BulletMoveInsideScreen)
{
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		bullet->SetPos({.x = 0.f, .y = 0.f});
		{
			//success bullet move down test, try to move inside screen bullet
			bullet->SetDirection(DOWN);
			const FPoint bulletStartPos = bullet->GetPos();
			_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);
			const FPoint bulletEndPos = bullet->GetPos();
			EXPECT_LT(bulletStartPos.y, bulletEndPos.y);
			EXPECT_EQ(bulletStartPos.x, bulletEndPos.x);
		}
		{
			//success bullet right test, try to move inside screen bullet
			bullet->SetDirection(RIGHT);
			const FPoint bulletStartPos = bullet->GetPos();
			_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);
			const FPoint bulletEndPos = bullet->GetPos();
			EXPECT_LT(bulletStartPos.x, bulletEndPos.x);
			EXPECT_EQ(bulletStartPos.y, bulletEndPos.y);
		}

		const float windowWidth = static_cast<float>(_windowSize.x);
		const float windowHeight = static_cast<float>(_windowSize.y);

		bullet->SetPos({.x = windowWidth - _bulletSize.x, .y = windowHeight - _bulletSize.y});
		{
			//success shot up test, try to create inside screen bullet
			bullet->SetDirection(UP);
			const FPoint bulletStartPos = bullet->GetPos();
			_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);
			const FPoint bulletEndPos = bullet->GetPos();
			EXPECT_GT(bulletStartPos.y, bulletEndPos.y);
			EXPECT_EQ(bulletStartPos.x, bulletEndPos.x);
		}
		{
			//success move left test, try to move inside screen bullet
			bullet->SetDirection(LEFT);
			const FPoint bulletStartPos = bullet->GetPos();
			_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);
			const FPoint bulletEndPos = bullet->GetPos();
			EXPECT_GT(bulletStartPos.x, bulletEndPos.x);
			EXPECT_EQ(bulletStartPos.y, bulletEndPos.y);

			return;
		}
	}

	EXPECT_TRUE(false);
}

// Check that bullet can't move outside of screen
TEST_F(BulletTest, BulletMoveOutSideScreen)
{
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		const float windowWidth = static_cast<float>(_windowSize.x);
		const float windowHeight = static_cast<float>(_windowSize.y);

		bullet->SetPos({.x = windowWidth - _bulletSize.x, .y = windowHeight - _bulletSize.y});
		{
			//fail bullet move down test, try to move outside screen bullet
			bullet->SetDirection(DOWN);
			bullet->SetPos({.x = windowWidth - _bulletSize.x, .y = windowHeight - _bulletSize.y});
			const FPoint bulletStartPos = bullet->GetPos();

			_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

			EXPECT_EQ(bulletStartPos, bullet->GetPos());
		}
		{
			//fail bullet move right test, try to move outside screen bullet
			bullet->SetDirection(RIGHT);
			bullet->SetPos({.x = windowWidth - _bulletSize.x, .y = windowHeight - _bulletSize.y});
			const FPoint bulletStartPos = bullet->GetPos();

			_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

			EXPECT_EQ(bulletStartPos, bullet->GetPos());
		}

		bullet->SetPos({.x = 0.f, .y = 0.f});
		{
			//fail bullet move up test, try to move outside screen bullet
			bullet->SetDirection(UP);
			const FPoint bulletStartPos = bullet->GetPos();

			_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

			EXPECT_EQ(bulletStartPos, bullet->GetPos());
		}
		{
			//fail bullet move left test, try to move outside screen bullet
			bullet->SetDirection(LEFT);
			const FPoint bulletStartPos = bullet->GetPos();

			_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

			EXPECT_EQ(bulletStartPos, bullet->GetPos());

			return;
		}
	}

	EXPECT_TRUE(false);
}

// Check that bullet can deal damage to other obstacle and self
TEST_F(BulletTest, BulletDamageBrickWhenMoveUp)
{
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		bullet->SetPos({.x = 0.f, .y = 7.f});
		bullet->SetDirection(UP);
		const ObjRectangle rect{.x = 0, .y = 0, .w = _gridSize, .h = _gridSize};
		_allObjects.emplace_back(std::make_shared<BrickWall>(rect, _windowBuffer, _windowSize, _events, 0));
		if (const auto brickWall = dynamic_cast<BrickWall*>(_allObjects.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int brickWallHealth = brickWall->GetHealth();

			_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

			EXPECT_GT(bulletHealth, bullet->GetHealth());
			EXPECT_GT(brickWallHealth, brickWall->GetHealth());

			return;
		}
	}

	EXPECT_TRUE(false);
}

// Check that bullet can deal damage to other obstacle and self
TEST_F(BulletTest, BulletDamageBrickWhenMoveLeft)
{
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		bullet->SetPos({.x = 7.f, .y = 0.f});
		bullet->SetDirection(LEFT);
		const ObjRectangle rect{.x = 0, .y = 0, .w = _gridSize, .h = _gridSize};
		_allObjects.emplace_back(std::make_shared<BrickWall>(rect, _windowBuffer, _windowSize, _events, 0));
		if (const auto brickWall = dynamic_cast<BrickWall*>(_allObjects.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int brickWallHealth = brickWall->GetHealth();

			_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

			EXPECT_GT(bulletHealth, bullet->GetHealth());
			EXPECT_GT(brickWallHealth, brickWall->GetHealth());

			return;
		}
	}

	EXPECT_TRUE(false);
}

// Check that bullet can deal damage to other obstacle and self
TEST_F(BulletTest, BulletDamageBrickWhenMoveDown)
{
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		bullet->SetPos({.x = 0.f, .y = 0.f});
		bullet->SetDirection(DOWN);
		const ObjRectangle rect{.x = 0.f, .y = 6.f, .w = _gridSize, .h = _gridSize};
		_allObjects.emplace_back(std::make_shared<BrickWall>(rect, _windowBuffer, _windowSize, _events, 0));
		if (const auto brick = dynamic_cast<BrickWall*>(_allObjects.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int brickHealth = brick->GetHealth();

			_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

			EXPECT_GT(bulletHealth, bullet->GetHealth());
			EXPECT_GT(brickHealth, brick->GetHealth());

			return;
		}
	}

	EXPECT_TRUE(false);
}

// Check that bullet can deal damage to other obstacle and self
TEST_F(BulletTest, BulletDamageBrickWhenMoveRight)
{
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		bullet->SetPos({.x = 0.f, .y = 0.f});
		bullet->SetDirection(RIGHT);
		const ObjRectangle rect{.x = 7.f, .y = 0.f, .w = _gridSize, .h = _gridSize};
		_allObjects.emplace_back(std::make_shared<BrickWall>(rect, _windowBuffer, _windowSize, _events, 0));
		if (const auto brick = dynamic_cast<BrickWall*>(_allObjects.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int brickHealth = brick->GetHealth();

			_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

			EXPECT_GT(bulletHealth, bullet->GetHealth());
			EXPECT_GT(brickHealth, brick->GetHealth());

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(BulletTest, BulletDamageTank)
{
	const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
	const float tankSize = gridSize * 3;// for better turns
	constexpr float tankSpeed{142};
	constexpr int tankHealth = 1;
	const ObjRectangle rect{.x = 0, .y = _bulletSize.y, .w = tankSize, .h = tankSize};
	constexpr int gray = 0x808080;
	auto currentGameMode = OnePlayer;
	auto bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _windowSize, _windowBuffer,
	                                               currentGameMode);
	_allObjects.emplace_back(
			std::make_shared<Enemy>(
					rect, gray, tankHealth, _windowBuffer, _windowSize, UP, tankSpeed, &_allObjects, _events, "Enemy",
					"EnemyTeam", bulletPool, false, 1));
	const auto enemy = dynamic_cast<Enemy*>(_allObjects.back().get());

	EXPECT_EQ(enemy->GetHealth(), 1);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(enemy->GetHealth(), 0);
}

// Check that bullet can deal damage to other bullet and self
TEST_F(BulletTest, BulletToBulletDamageEachOther)
{
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		const ObjRectangle rect{.x = 0, .y = _bulletSize.y + 1, .w = _bulletSize.x, .h = _bulletSize.y};
		_allObjects.emplace_back(
				std::make_shared<Bullet>(
						rect, damage, bulletDamageAreaRadius, color, health, _windowBuffer, _windowSize, UP,
						_bulletSpeed, &_allObjects, _events, "Player2", "PlayerTeam", 1, false));

		if (const auto bullet2 = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int bullet2Health = bullet2->GetHealth();

			_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

			EXPECT_GT(bulletHealth, bullet->GetHealth());
			EXPECT_GT(bullet2Health, bullet2->GetHealth());

			return;
		}
	}

	EXPECT_TRUE(false);
}

// Check that bullet can't deal damage to undestractable object
TEST_F(BulletTest, BulletCantDamageSteelWall)
{
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		bullet->SetPos({.x = 0.f, .y = 0.f});
		bullet->SetDirection(DOWN);
		const ObjRectangle rect{.x = 0.f, .y = 6.f, .w = _gridSize, .h = _gridSize};
		_allObjects.emplace_back(std::make_shared<SteelWall>(rect, _windowBuffer, _windowSize, _events, 0));
		if (const auto brick = dynamic_cast<SteelWall*>(_allObjects.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int brickHealth = brick->GetHealth();

			_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

			EXPECT_GT(bulletHealth, bullet->GetHealth());
			EXPECT_EQ(brickHealth, brick->GetHealth());

			return;
		}
	}

	EXPECT_TRUE(false);
}

// Check that bullet can pass through water
TEST_F(BulletTest, BulletCantDamageWater)
{
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		bullet->SetPos({.x = 0.f, .y = 0.f});
		bullet->SetDirection(DOWN);
		const ObjRectangle rect{.x = 0.f, .y = 6.f, .w = _gridSize, .h = _gridSize};
		_allObjects.emplace_back(std::make_shared<WaterTile>(rect, _windowBuffer, _windowSize, _events, 0));
		if (const auto brick = dynamic_cast<WaterTile*>(_allObjects.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int brickHealth = brick->GetHealth();

			_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

			EXPECT_EQ(bulletHealth, bullet->GetHealth());
			EXPECT_EQ(brickHealth, brick->GetHealth());

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(BulletTest, BulletDamagefortressWall)
{
	if (auto&& bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		bullet->SetPos({.x = 0.f, .y = 0.f});
		bullet->SetDirection(DOWN);
		const ObjRectangle rect{.x = 0.f, .y = 6.f, .w = _gridSize, .h = _gridSize};
		_allObjects.emplace_back(
				std::make_shared<FortressWall>(rect, _windowBuffer, _windowSize, _events, &_allObjects, 0));
		if (const auto fortressWall = dynamic_cast<FortressWall*>(_allObjects.back().get()))
		{
			fortressWall->SetHealth(1);
			EXPECT_EQ(fortressWall->GetHealth(), 1);

			_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

			EXPECT_EQ(fortressWall->GetHealth(), 0);

			return;
		}
	}

	EXPECT_TRUE(false);
}
