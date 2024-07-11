#include "../headers/EventSystem.h"
#include "../headers/Point.h"
#include "../headers/obstacles/Brick.h"
#include "../headers/obstacles/Iron.h"
#include "../headers/obstacles/Water.h"
#include "../headers/pawns/Bullet.h"
#include "../headers/pawns/Enemy.h"

#include "gtest/gtest.h"

#include <memory>

class BulletTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events;
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	UPoint _windowSize{800, 600};
	FPoint _bulletSize;
	float _bulletSpeed{300.f};
	int* _windowBuffer{nullptr};
	float _gridSize = 1;

	void SetUp() override
	{
		_gridSize = static_cast<float>(_windowSize.y) / 50.f;
		_events = std::make_shared<EventSystem>();
		_bulletSize = FPoint{6.f, 5.f};
		Rectangle bulletRect{0.f, 0.f, _bulletSize.x, _bulletSize.y};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		std::string name = "PlayerOne";
		std::string fraction = "PlayerTeam";
		_allObjects.reserve(4);
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, DOWN, _bulletSpeed, &_allObjects,
		                                                  _events, name, fraction));
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

		bullet->SetPos({static_cast<float>(_windowSize.x), static_cast<float>(_windowSize.y)});

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
		constexpr float deltaTime = 1.f / 60.f;

		bullet->SetPos({0.f, 0.f});
		{
			//success bullet move down test, try to move inside screen bullet
			bullet->SetDirection(DOWN);
			const FPoint bulletStartPos = bullet->GetPos();
			_events->EmitEvent<const float>("TickUpdate", deltaTime);
			const FPoint bulletEndPos = bullet->GetPos();
			EXPECT_LT(bulletStartPos.y, bulletEndPos.y);
			EXPECT_EQ(bulletStartPos.x, bulletEndPos.x);
		}
		{
			//success bullet right test, try to move inside screen bullet
			bullet->SetDirection(RIGHT);
			const FPoint bulletStartPos = bullet->GetPos();
			_events->EmitEvent<const float>("TickUpdate", deltaTime);
			const FPoint bulletEndPos = bullet->GetPos();
			EXPECT_LT(bulletStartPos.x, bulletEndPos.x);
			EXPECT_EQ(bulletStartPos.y, bulletEndPos.y);
		}

		const float windowWidth = static_cast<float>(_windowSize.x);
		const float windowHeight = static_cast<float>(_windowSize.y);

		bullet->SetPos({windowWidth - _bulletSize.x, windowHeight - _bulletSize.y});
		{
			//success shot up test, try to create inside screen bullet
			bullet->SetDirection(UP);
			const FPoint bulletStartPos = bullet->GetPos();
			_events->EmitEvent<const float>("TickUpdate", deltaTime);
			const FPoint bulletEndPos = bullet->GetPos();
			EXPECT_GT(bulletStartPos.y, bulletEndPos.y);
			EXPECT_EQ(bulletStartPos.x, bulletEndPos.x);
		}
		{
			//success move left test, try to move inside screen bullet
			bullet->SetDirection(LEFT);
			const FPoint bulletStartPos = bullet->GetPos();
			_events->EmitEvent<const float>("TickUpdate", deltaTime);
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
		constexpr float deltaTime = 1.f / 60.f;
		const float windowWidth = static_cast<float>(_windowSize.x);
		const float windowHeight = static_cast<float>(_windowSize.y);

		bullet->SetPos({windowWidth - _bulletSize.x, windowHeight - _bulletSize.y});
		{
			//fail bullet move down test, try to move outside screen bullet
			bullet->SetDirection(DOWN);
			bullet->SetPos({windowWidth - _bulletSize.x, windowHeight - _bulletSize.y});
			const FPoint bulletStartPos = bullet->GetPos();

			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(bulletStartPos, bullet->GetPos());
		}
		{
			//fail bullet move right test, try to move outside screen bullet
			bullet->SetDirection(RIGHT);
			bullet->SetPos({windowWidth - _bulletSize.x, windowHeight - _bulletSize.y});
			const FPoint bulletStartPos = bullet->GetPos();

			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(bulletStartPos, bullet->GetPos());
		}

		bullet->SetPos({0.f, 0.f});
		{
			//fail bullet move up test, try to move outside screen bullet
			bullet->SetDirection(UP);
			const FPoint bulletStartPos = bullet->GetPos();

			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(bulletStartPos, bullet->GetPos());
		}
		{
			//fail bullet move left test, try to move outside screen bullet
			bullet->SetDirection(LEFT);
			const FPoint bulletStartPos = bullet->GetPos();

			_events->EmitEvent<const float>("TickUpdate", deltaTime);

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
		bullet->SetPos({0.f, 7.f});
		bullet->SetDirection(UP);
		const Rectangle rect{0, 0, _gridSize, _gridSize};
		_allObjects.emplace_back(std::make_shared<Brick>(rect, _windowBuffer, _windowSize, _events));
		if (const auto brick = dynamic_cast<Brick*>(_allObjects.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int brickHealth = brick->GetHealth();

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_GT(bulletHealth, bullet->GetHealth());
			EXPECT_GT(brickHealth, brick->GetHealth());

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
		bullet->SetPos({7.f, 0.f});
		bullet->SetDirection(LEFT);
		const Rectangle rect{0, 0, _gridSize, _gridSize};
		_allObjects.emplace_back(std::make_shared<Brick>(rect, _windowBuffer, _windowSize, _events));
		if (const auto brick = dynamic_cast<Brick*>(_allObjects.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int brickHealth = brick->GetHealth();

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_GT(bulletHealth, bullet->GetHealth());
			EXPECT_GT(brickHealth, brick->GetHealth());

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
		bullet->SetPos({0.f, 0.f});
		bullet->SetDirection(DOWN);
		const Rectangle rect{0.f, 6.f, _gridSize, _gridSize};
		_allObjects.emplace_back(std::make_shared<Brick>(rect, _windowBuffer, _windowSize, _events));
		if (const auto brick = dynamic_cast<Brick*>(_allObjects.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int brickHealth = brick->GetHealth();

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

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
		bullet->SetPos({0.f, 0.f});
		bullet->SetDirection(RIGHT);
		const Rectangle rect{7.f, 0.f, _gridSize, _gridSize};
		_allObjects.emplace_back(std::make_shared<Brick>(rect, _windowBuffer, _windowSize, _events));
		if (const auto brick = dynamic_cast<Brick*>(_allObjects.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int brickHealth = brick->GetHealth();

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_GT(bulletHealth, bullet->GetHealth());
			EXPECT_GT(brickHealth, brick->GetHealth());

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(BulletTest, BulletDamageTank)
{
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
		const float tankSize = gridSize * 3;// for better turns
		constexpr float tankSpeed{142};
		constexpr int tankHealth = 100;
		const Rectangle rect{0, _bulletSize.y, tankSize, tankSize};
		constexpr int gray = 0x808080;
		auto bulletPool = std::make_shared<BulletPool>();
		_allObjects.emplace_back(std::make_shared<Enemy>(rect, gray, tankHealth, _windowBuffer, _windowSize, UP,
		                                                 tankSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam",
		                                                 bulletPool));
		if (const auto enemy = dynamic_cast<Enemy*>(_allObjects.back().get()))
		{
			enemy->SetHealth(1);
			EXPECT_EQ(enemy->GetHealth(), 1);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(enemy->GetHealth(), 0);

			return;
		}
	}

	EXPECT_TRUE(false);
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
		const Rectangle rect{0, _bulletSize.y + 1, _bulletSize.x, _bulletSize.y};
		_allObjects.emplace_back(std::make_shared<Bullet>(rect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "PlayerTwo", "PlayerTeam"));

		if (const auto bullet2 = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int bullet2Health = bullet2->GetHealth();

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_GT(bulletHealth, bullet->GetHealth());
			EXPECT_GT(bullet2Health, bullet2->GetHealth());

			return;
		}
	}

	EXPECT_TRUE(false);
}

// Check that bullet can't deal damage to undestractable object
TEST_F(BulletTest, BulletCantDamageIron)
{
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		bullet->SetPos({0.f, 0.f});
		bullet->SetDirection(DOWN);
		const Rectangle rect{0.f, 6.f, _gridSize, _gridSize};
		_allObjects.emplace_back(std::make_shared<Iron>(rect, _windowBuffer, _windowSize, _events));
		if (const auto brick = dynamic_cast<Iron*>(_allObjects.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int brickHealth = brick->GetHealth();

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

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
		bullet->SetPos({0.f, 0.f});
		bullet->SetDirection(DOWN);
		const Rectangle rect{0.f, 6.f, _gridSize, _gridSize};
		_allObjects.emplace_back(std::make_shared<Water>(rect, _windowBuffer, _windowSize, _events));
		if (const auto brick = dynamic_cast<Water*>(_allObjects.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int brickHealth = brick->GetHealth();

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(bulletHealth, bullet->GetHealth());
			EXPECT_EQ(brickHealth, brick->GetHealth());

			return;
		}
	}

	EXPECT_TRUE(false);
}
