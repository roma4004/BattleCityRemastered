#include "../headers/Point.h"
#include "../headers/application/Window.h"
#include "../headers/components/BulletPool.h"
#include "../headers/components/EventSystem.h"
#include "../headers/enums/Direction.h"
#include "../headers/enums/GameMode.h"
#include "../headers/obstacles/BrickWall.h"
#include "../headers/obstacles/FortressWall.h"
#include "../headers/obstacles/SteelWall.h"
#include "../headers/obstacles/WaterTile.h"
#include "../headers/pawns/Bullet.h"
#include "../headers/pawns/Enemy.h"
#include "../headers/pawns/PawnProperty.h"

#include "gtest/gtest.h"

#include <memory>

class BulletTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<Window> _window{nullptr};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	FPoint _bulletSize;
	float _bulletSpeed{300.f};
	float _gridSize{1};
	float _deltaTimeOneFrame{1.f / 60.f};
	GameMode _gameMode{OnePlayer};
	int _bulletDamage{1};
	int _bulletHealth{1};
	int _bulletColor{0xffffff};
	float _tankSize{0.f};
	float _tankSpeed{142.f};
	float _bulletWidth{6.f};
	float _bulletHeight{5.f};
	double _bulletDamageRadius{12.0};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_window = std::make_shared<Window>(UPoint{.x = 800, .y = 600}, std::shared_ptr<int[]>());
		_gridSize = static_cast<float>(_window->size.y) / 50.f;
		_bulletSize = FPoint{.x = 6.f, .y = 5.f};

		std::string name{"Bullet1"};
		std::string fraction{"PlayerTeam"};
		std::string author{"Player1"};
		ObjRectangle rect{.x = 0.f, .y = 0.f, .w = _bulletSize.x, .h = _bulletSize.y};
		BaseObjProperty baseObjProperty{
			std::move(rect), _bulletColor, _bulletHealth, true, 1, std::move(name), std::move(fraction)};
		PawnProperty pawnProperty{
			std::move(baseObjProperty), _window, DOWN, _bulletSpeed, &_allObjects, _events, 1, _gameMode};

		_allObjects.reserve(4);
		_allObjects.emplace_back(
				std::make_shared<Bullet>(std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author)));
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

		bullet->SetPos({.x = static_cast<float>(_window->size.x), .y = static_cast<float>(_window->size.y)});

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

		const float windowWidth = static_cast<float>(_window->size.x);
		const float windowHeight = static_cast<float>(_window->size.y);

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
		const float windowWidth = static_cast<float>(_window->size.x);
		const float windowHeight = static_cast<float>(_window->size.y);

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
		ObjRectangle rect{.x = 0, .y = 0, .w = _gridSize, .h = _gridSize};
		_allObjects.emplace_back(std::make_shared<BrickWall>(std::move(rect), _window, _events, 0, _gameMode));
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
		ObjRectangle rect{.x = 0, .y = 0, .w = _gridSize, .h = _gridSize};
		_allObjects.emplace_back(std::make_shared<BrickWall>(std::move(rect), _window, _events, 0, _gameMode));
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
		ObjRectangle rect{.x = 0.f, .y = 6.f, .w = _gridSize, .h = _gridSize};
		_allObjects.emplace_back(std::make_shared<BrickWall>(std::move(rect), _window, _events, 0, _gameMode));
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
		ObjRectangle rect{.x = 7.f, .y = 0.f, .w = _gridSize, .h = _gridSize};
		_allObjects.emplace_back(std::make_shared<BrickWall>(std::move(rect), _window, _events, 0, _gameMode));
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
	const float gridSize = static_cast<float>(_window->size.y) / 50.f;
	const float tankSize = gridSize * 3;// for better turns
	constexpr int tankHealth = 1;
	constexpr int gray = 0x808080;
	auto bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _window, _gameMode);

	ObjRectangle rect{.x = 0, .y = _bulletSize.y, .w = tankSize, .h = tankSize};
	BaseObjProperty baseObjProperty{std::move(rect), gray, tankHealth, true, 1, "Enemy1", "EnemyTeam"};
	PawnProperty pawnProperty{
		std::move(baseObjProperty), _window, UP, _tankSpeed, &_allObjects, _events, 1, _gameMode};

	_allObjects.emplace_back(std::make_shared<Enemy>(std::move(pawnProperty), std::move(bulletPool)));

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
		std::string name{"Bullet2"};
		std::string fraction{"PlayerTeam"};
		std::string author{"Player2"};
		ObjRectangle rect{.x = 0, .y = _bulletSize.y + 1, .w = _bulletSize.x, .h = _bulletSize.y};
		BaseObjProperty baseObjProperty{
			std::move(rect), _bulletColor, _bulletHealth, true, 1, std::move(name), std::move(fraction)};
		PawnProperty pawnProperty{
			std::move(baseObjProperty), _window, UP, _bulletSpeed, &_allObjects, _events, 1, _gameMode};

		_allObjects.emplace_back(
				std::make_shared<Bullet>(std::move(pawnProperty), _bulletDamage, _bulletDamageRadius, std::move(author)));

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
		ObjRectangle rect{.x = 0.f, .y = 6.f, .w = _gridSize, .h = _gridSize};
		_allObjects.emplace_back(std::make_shared<SteelWall>(std::move(rect), _window, _events, 0, _gameMode));
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
		ObjRectangle rect{.x = 0.f, .y = 6.f, .w = _gridSize, .h = _gridSize};
		_allObjects.emplace_back(std::make_shared<WaterTile>(std::move(rect), _window, _events, 0, _gameMode));
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
		ObjRectangle rect{.x = 0.f, .y = 6.f, .w = 36, .h = 36};
		_allObjects.emplace_back(
				std::make_shared<FortressWall>(std::move(rect), _window, _events, &_allObjects, 0, _gameMode));
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

//TODO: write test that Bullet with _tier > 2 can destroyed SteelWall
