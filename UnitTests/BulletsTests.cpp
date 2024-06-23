#include "MockBullet.h"
#include "../headers/Brick.h"
#include "../headers/Bullet.h"
#include "../headers/EventSystem.h"

#include "gtest/gtest.h"

#include <memory>

// Класс для тестирования
class BulletTest : public ::testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events;
	std::vector<std::shared_ptr<BaseObj>> allPawns;
	UPoint _windowSize{800, 600};
	float _tankSize{};
	float _tankSpeed{142};
	float _bulletSpeed{300.f};
	int* _windowBuffer{nullptr};
	int _tankHealth = 100;
	float _gridSize = 1;

	void SetUp() override
	{
		_gridSize = static_cast<float>(_windowSize.y) / 50.f;
		_events = std::make_shared<EventSystem>();
		constexpr Direction direction = Direction::DOWN;
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		constexpr Rectangle bulletRect{0, 0, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		allPawns.emplace_back(std::make_shared<MockBullet>(bulletRect, damage, bulletDamageAreaRadius, color,
		                                                   _bulletSpeed, direction, health, nullptr, _windowSize,
		                                                   &allPawns, _events));
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
	if (const auto bullet = dynamic_cast<MockBullet*>(allPawns.back().get()))
	{
		bullet->SetPos({});
		const FPoint startPos = bullet->GetPos();

		bullet->SetPos({static_cast<float>(_windowSize.x), static_cast<float>(_windowSize.y)});

		EXPECT_LT(startPos, bullet->GetPos());
	}
}

// Check that tank can shoot inside of screen and can't outside
TEST_F(BulletTest, BulletMoveInsideScreen)
{
	if (const auto bullet = dynamic_cast<MockBullet*>(allPawns.back().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;

		bullet->SetPos({0.f, 0.f});
		{
			//success bullet move down test, try to move inside screen bullet
			bullet->SetDirection(DOWN);
			const FPoint bulletStartPos = bullet->GetPos();
			bullet->Move(deltaTime);
			const FPoint bulletEndPos = bullet->GetPos();
			EXPECT_LT(bulletStartPos.y, bulletEndPos.y);
			EXPECT_EQ(bulletStartPos.x, bulletEndPos.x);
		}
		{
			//success bullet right test, try to move inside screen bullet
			bullet->SetDirection(RIGHT);
			const FPoint bulletStartPos = bullet->GetPos();
			bullet->Move(deltaTime);
			const FPoint bulletEndPos = bullet->GetPos();
			EXPECT_LT(bulletStartPos.x, bulletEndPos.x);
			EXPECT_EQ(bulletStartPos.y, bulletEndPos.y);
		}

		const float windowWidth = static_cast<float>(_windowSize.x);
		const float windowHeight = static_cast<float>(_windowSize.y);

		bullet->SetPos({windowWidth - _tankSize, windowHeight - _tankSize});
		{
			//success shot up test, try to create inside screen bullet
			bullet->SetDirection(UP);
			const FPoint bulletStartPos = bullet->GetPos();
			bullet->Move(deltaTime);
			const FPoint bulletEndPos = bullet->GetPos();
			EXPECT_GT(bulletStartPos.y, bulletEndPos.y);
			EXPECT_EQ(bulletStartPos.x, bulletEndPos.x);
		}
		{
			//success move left test, try to move inside screen bullet
			bullet->SetDirection(LEFT);
			const FPoint bulletStartPos = bullet->GetPos();
			bullet->Move(deltaTime);
			const FPoint bulletEndPos = bullet->GetPos();
			EXPECT_GT(bulletStartPos.x, bulletEndPos.x);
			EXPECT_EQ(bulletStartPos.y, bulletEndPos.y);
		}
	}
}

// Check that tank can shoot inside of screen and can't outside
TEST_F(BulletTest, BulletMoveOutSide)
{
	if (const auto bullet = dynamic_cast<MockBullet*>(allPawns.back().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;
		const float windowWidth = static_cast<float>(_windowSize.x);
		const float windowHeight = static_cast<float>(_windowSize.y);

		bullet->SetPos({0.f, 0.f});
		{
			//fail bullet move down test, try to move outside screen bullet
			bullet->SetDirection(DOWN);
			const float bulletWidth = bullet->GetWidth();
			const float bulletHeight = bullet->GetHeight();
			bullet->SetPos({windowWidth - bulletWidth, windowHeight - bulletHeight});
			const FPoint bulletStart = bullet->GetPos();
			bullet->Move(deltaTime);
			EXPECT_EQ(bulletStart, bullet->GetPos());
		}
		{
			//fail bullet move right test, try to move outside screen bullet
			bullet->SetDirection(RIGHT);
			const float bulletWidth = bullet->GetWidth();
			const float bulletHeight = bullet->GetHeight();
			bullet->SetPos({windowWidth - bulletWidth, windowHeight - bulletHeight});
			const FPoint bulletStartPos = bullet->GetPos();
			bullet->Move(deltaTime);
			EXPECT_EQ(bulletStartPos, bullet->GetPos());
		}

		bullet->SetPos({windowWidth - _tankSize, windowHeight - _tankSize});
		{
			//fail bullet move up test, try to move outside screen bullet
			bullet->SetDirection(UP);
			const FPoint bulletStart = bullet->GetPos();
			bullet->Move(deltaTime);
			EXPECT_EQ(bulletStart, bullet->GetPos());
		}
		{
			//fail bullet move left test, try to move outside screen bullet
			bullet->SetDirection(LEFT);
			const FPoint bulletStart = bullet->GetPos();
			bullet->Move(deltaTime);
			EXPECT_EQ(bulletStart, bullet->GetPos());
		}
	}
}

// Check that bullet can deal damage to other obstacle and self
TEST_F(BulletTest, BulletDamageWhenMoveUp)
{
	if (const auto bullet = dynamic_cast<MockBullet*>(allPawns.back().get()))
	{
		bullet->SetPos({0.f, 7.f});
		bullet->SetDirection(UP);
		const Rectangle rect{0, 0, _gridSize, _gridSize};
		allPawns.emplace_back(std::make_shared<Brick>(rect, _windowBuffer, _windowSize, _events));
		if (const auto brick = dynamic_cast<Brick*>(allPawns.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int brickHealth = brick->GetHealth();
			constexpr float deltaTime = 1.f / 60.f;
			bullet->Move(deltaTime);
			EXPECT_GT(bulletHealth, bullet->GetHealth());
			EXPECT_GT(brickHealth, brick->GetHealth());
		}
	}
}

// Check that bullet can deal damage to other obstacle and self
TEST_F(BulletTest, BulletDamageWhenMoveLeft)
{
	if (const auto bullet = dynamic_cast<MockBullet*>(allPawns.back().get()))
	{
		bullet->SetPos({7.f, 0.f});
		bullet->SetDirection(LEFT);
		const Rectangle rect{0, 0, _gridSize, _gridSize};
		allPawns.emplace_back(std::make_shared<Brick>(rect, _windowBuffer, _windowSize, _events));
		if (const auto brick = dynamic_cast<Brick*>(allPawns.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int brickHealth = brick->GetHealth();
			constexpr float deltaTime = 1.f / 60.f;
			bullet->Move(deltaTime);
			EXPECT_GT(bulletHealth, bullet->GetHealth());
			EXPECT_GT(brickHealth, brick->GetHealth());
		}
	}
}

// Check that bullet can deal damage to other obstacle and self
TEST_F(BulletTest, BulletDamageWhenMoveDown)
{
	if (const auto bullet = dynamic_cast<MockBullet*>(allPawns.back().get()))
	{
		bullet->SetPos({0.f, 0.f});
		bullet->SetDirection(DOWN);
		const Rectangle rect{0.f, 6.f, _gridSize, _gridSize};
		allPawns.emplace_back(std::make_shared<Brick>(rect, _windowBuffer, _windowSize, _events));
		if (const auto brick = dynamic_cast<Brick*>(allPawns.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int brickHealth = brick->GetHealth();
			constexpr float deltaTime = 1.f / 60.f;
			bullet->Move(deltaTime);
			EXPECT_GT(bulletHealth, bullet->GetHealth());
			EXPECT_GT(brickHealth, brick->GetHealth());
		}
	}
}

// Check that bullet can deal damage to other obstacle and self
TEST_F(BulletTest, BulletDamageWhenMoveRight)
{
	if (const auto bullet = dynamic_cast<MockBullet*>(allPawns.back().get()))
	{
		bullet->SetPos({0.f, 0.f});
		bullet->SetDirection(RIGHT);
		const Rectangle rect{7.f, 0.f, _gridSize, _gridSize};
		allPawns.emplace_back(std::make_shared<Brick>(rect, _windowBuffer, _windowSize, _events));
		if (const auto brick = dynamic_cast<Brick*>(allPawns.back().get()))
		{
			const int bulletHealth = bullet->GetHealth();
			const int brickHealth = brick->GetHealth();
			constexpr float deltaTime = 1.f / 60.f;
			bullet->Move(deltaTime);
			EXPECT_GT(bulletHealth, bullet->GetHealth());
			EXPECT_GT(brickHealth, brick->GetHealth());
		}
	}
}

// Check that bullet can deal damage to other bullet and self
TEST_F(BulletTest, BulletToBulletDamageEachOther)
{
	constexpr Direction direction = Direction::DOWN;
	constexpr float bulletWidth = 6;
	constexpr float bulletHeight = 5;
	constexpr Rectangle bulletRect{0, 0, bulletWidth, bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	allPawns.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, _bulletSpeed,
	                                               direction, health, _windowBuffer, _windowSize, &allPawns, _events));
	if (const auto bullet = dynamic_cast<Bullet*>(allPawns.back().get()))
	{
		const int bulletHealth = bullet->GetHealth();

		constexpr Rectangle bulletRect2{0, 6, bulletWidth, bulletHeight};
		constexpr Direction direction2 = Direction::UP;
		allPawns.emplace_back(std::make_shared<Bullet>(bulletRect2, damage, bulletDamageAreaRadius, color, _bulletSpeed,
		                                               direction2, health, _windowBuffer, _windowSize, &allPawns,
		                                               _events));

		if (const auto bullet2 = dynamic_cast<Bullet*>(allPawns.back().get()))
		{
			const int bullet2Health = bullet2->GetHealth();
			constexpr float deltaTime = 1.f / 60.f;
			bullet2->Move(deltaTime);
			EXPECT_GT(bulletHealth, bullet->GetHealth());
			EXPECT_GT(bullet2Health, bullet2->GetHealth());
		}
	}
}

//TODO: write collision test between:
//bullet and tank
