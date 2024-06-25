#include "MockPlayerOne.h"
#include "../headers/Bullet.h"
#include "../headers/EventSystem.h"

#include "gtest/gtest.h"

#include <memory>

// TEST(TestCaseName, TestName)
// {
// 	EXPECT_EQ(1, 1);
// 	EXPECT_TRUE(true);
// }


// Класс для тестирования
class PlayerTest : public ::testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events;
	std::vector<std::shared_ptr<BaseObj>> allPawns;
	UPoint _windowSize{800, 600};
	float _tankSize{};
	float _tankSpeed{142};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		constexpr int tankHealth = 100;
		const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
		_tankSize = gridSize * 3;// for better turns
		int* windowBuffer{nullptr};
		Rectangle playerRect{0, 0, _tankSize, _tankSize};
		allPawns.emplace_back(std::make_shared<MockPlayerOne>(playerRect, 0xeaea00, _tankSpeed, tankHealth,
		                                                      windowBuffer, _windowSize, &allPawns, _events));
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

// Check that tank can move inside screen
TEST_F(PlayerTest, TankMoveInSideScreen)
{
	if (const auto player = dynamic_cast<MockPlayerOne*>(allPawns.back().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;

		{
			//moveUp test
			player->SetPos({0.f, _tankSize + _tankSpeed});
			const FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->MockMoveKeyPressed(UP);
			player->Move(deltaTime);
			const FPoint endPos = player->GetPos();
			EXPECT_NE(startPos.y, endPos.y);
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_GT(startPos.y, endPos.y);
			// EXPECT_CALL(*player, Move(1.f)).Times(1);
		}
		{
			//moveUp Left
			player->SetPos({_tankSize + _tankSpeed, 0.f});
			const FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->MockMoveKeyPressed(LEFT);
			player->Move(deltaTime);
			const FPoint endPos = player->GetPos();
			EXPECT_NE(startPos.x, endPos.x);
			EXPECT_EQ(startPos.y, endPos.y);
			EXPECT_GT(startPos.x, endPos.x);
			// EXPECT_CALL(*player, Move(1.f)).Times(1);
		}

		const float windowWidth = static_cast<float>(_windowSize.x);
		const float windowHeight = static_cast<float>(_windowSize.y);

		{
			//moveUp Down
			player->SetPos({windowWidth - _tankSize, windowHeight - _tankSize - _tankSpeed});
			const FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->MockMoveKeyPressed(DOWN);
			player->Move(deltaTime);
			const FPoint endPos = player->GetPos();
			EXPECT_NE(startPos.y, endPos.y);
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_LT(startPos.y, endPos.y);
			// EXPECT_CALL(*player, Move(1.f)).Times(1);
		}
		{
			//moveUp Right
			player->SetPos({windowWidth - _tankSize - _tankSpeed, windowHeight - _tankSize});
			const FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->MockMoveKeyPressed(RIGHT);
			player->Move(deltaTime);
			const FPoint endPos = player->GetPos();
			EXPECT_NE(startPos.x, endPos.x);
			EXPECT_EQ(startPos.y, endPos.y);
			EXPECT_LT(startPos.x, endPos.x);
			// EXPECT_CALL(*player, Move(1.f)).Times(1);
		}
	}
}

// Check that tank cannot move out of screen
TEST_F(PlayerTest, TankMoveOutSideScreen)
{
	if (const auto player = dynamic_cast<MockPlayerOne*>(allPawns.back().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;

		{
			//fail moveUp test
			player->SetPos({0.f, 0.f});
			const FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->MockMoveKeyPressed(UP);
			player->Move(deltaTime);
			EXPECT_EQ(startPos, player->GetPos());
			// EXPECT_CALL(*player, Move(1.f)).Times(1);
		}
		{
			//fail  moveUp Left
			player->SetPos({0.f, 0.f});
			const FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->MockMoveKeyPressed(LEFT);
			player->Move(deltaTime);
			EXPECT_EQ(startPos, player->GetPos());
			// EXPECT_CALL(*player, Move(1.f)).Times(1);
		}

		const float windowWidth = static_cast<float>(_windowSize.x);
		const float windowHeight = static_cast<float>(_windowSize.y);
		{
			//fail moveUp Down
			player->SetPos({windowWidth - _tankSize, windowHeight - _tankSize});
			const FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->MockMoveKeyPressed(DOWN);
			player->Move(deltaTime);
			EXPECT_EQ(startPos, player->GetPos());
			// EXPECT_CALL(*player, Move(1.f)).Times(1);
		}
		{
			//fail moveUp Right
			player->SetPos({windowWidth - _tankSize, windowHeight - _tankSize});
			const FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->MockMoveKeyPressed(RIGHT);
			player->Move(deltaTime);
			EXPECT_EQ(startPos, player->GetPos());
			// EXPECT_CALL(*player, Move(1.f)).Times(1);
		}
	}
}

// Check that tank set their position correctly
TEST_F(PlayerTest, TankSetPos)
{
	if (const auto player = dynamic_cast<MockPlayerOne*>(allPawns.back().get()))
	{
		player->SetPos({});
		const FPoint startPos = player->GetPos();

		player->SetPos({static_cast<float>(_windowSize.x), static_cast<float>(_windowSize.y)});

		EXPECT_LT(startPos, player->GetPos());
	}
}

// Check that tank don't move when shooting
TEST_F(PlayerTest, TankDontMoveWhenShot)
{
	if (const auto player = dynamic_cast<MockPlayerOne*>(allPawns.back().get()))
	{
		player->SetPos({static_cast<float>(_windowSize.x) / 2.f, static_cast<float>(_windowSize.y) / 2.f});
		const FPoint startPos = player->GetPos();
		{
			player->SetDirection(UP);
			player->Shot();
			EXPECT_EQ(startPos, player->GetPos());
		}
		{
			player->SetDirection(LEFT);
			player->Shot();
			EXPECT_EQ(startPos, player->GetPos());
		}
		{
			player->SetDirection(DOWN);
			player->Shot();
			EXPECT_EQ(startPos, player->GetPos());
		}
		{
			player->SetDirection(RIGHT);
			player->Shot();
			EXPECT_EQ(startPos, player->GetPos());
		}
	}
}

// Check that tank can shoot inside of screen
TEST_F(PlayerTest, TankShotInSideScreen)
{
	if (const auto player = dynamic_cast<MockPlayerOne*>(allPawns.back().get()))
	{
		player->SetPos({0.f, 0.f});
		{
			//success shot down test, try to create inside screen bullet
			const size_t size = allPawns.size();
			player->SetDirection(DOWN);
			player->Shot();
			EXPECT_LT(size, allPawns.size());
		}
		{
			//success shot right test, try to create inside screen bullet
			const size_t size = allPawns.size();
			player->SetDirection(RIGHT);
			player->Shot();
			EXPECT_LT(size, allPawns.size());
		}

		player->SetPos({static_cast<float>(_windowSize.x) - _tankSize, static_cast<float>(_windowSize.y) - _tankSize});
		{
			//success shot up test, try to create inside screen bullet
			const size_t size = allPawns.size();
			player->SetDirection(UP);
			player->Shot();
			EXPECT_LT(size, allPawns.size());
		}
		{
			//success shot left test, try to create inside screen bullet
			const size_t size = allPawns.size();
			player->SetDirection(LEFT);
			player->Shot();
			EXPECT_LT(size, allPawns.size());
		}
	}
}

// Check that tank can't shoot outside of screen
TEST_F(PlayerTest, TankShotOutSideScreen)
{
	if (const auto player = dynamic_cast<MockPlayerOne*>(allPawns.back().get()))
	{
		player->SetPos({0.f, 0.f});
		{
			//fail shot up test, try to create outside screen bullet
			const size_t size = allPawns.size();
			player->MockResetKeyPressed();
			player->SetDirection(UP);
			player->MockShotKeyPressed();
			player->Shot();
			EXPECT_EQ(size, allPawns.size());
		}
		{
			//fail shot left test, try to create outside screen bullet
			const size_t size = allPawns.size();
			player->SetDirection(LEFT);
			player->Shot();
			EXPECT_EQ(size, allPawns.size());
		}

		player->SetPos({static_cast<float>(_windowSize.x) - _tankSize, static_cast<float>(_windowSize.y) - _tankSize});
		{
			//fail shot down test, try to create outside screen bullet
			const size_t size = allPawns.size();
			player->SetDirection(DOWN);
			player->Shot();
			EXPECT_EQ(size, allPawns.size());
		}
		{
			//fail shot right test, try to create outside screen bullet
			const size_t size = allPawns.size();
			player->SetDirection(RIGHT);
			player->Shot();
			EXPECT_EQ(size, allPawns.size());
		}
	}
}

// Check that tank can shoot inside of screen and can't outside
TEST_F(PlayerTest, BulletMoveInsideScreen)
{
	if (const auto player = dynamic_cast<MockPlayerOne*>(allPawns.back().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;

		player->SetPos({0.f, 0.f});
		{
			//success bullet move down test, try to move inside screen bullet
			player->SetDirection(DOWN);
			player->Shot();
			if (const auto bullet = dynamic_cast<Bullet*>(allPawns.back().get()))
			{
				const FPoint bulletStartPos = bullet->GetPos();
				bullet->Move(deltaTime);
				const FPoint bulletEndPos = bullet->GetPos();
				EXPECT_LT(bulletStartPos.y, bulletEndPos.y);
				EXPECT_EQ(bulletStartPos.x, bulletEndPos.x);
			}
		}
		{
			//success bullet right test, try to move inside screen bullet
			player->SetDirection(RIGHT);
			player->Shot();
			if (const auto bullet = dynamic_cast<Bullet*>(allPawns.back().get()))
			{
				const FPoint bulletStartPos = bullet->GetPos();
				bullet->Move(deltaTime);
				const FPoint bulletEndPos = bullet->GetPos();
				EXPECT_LT(bulletStartPos.x, bulletEndPos.x);
				EXPECT_EQ(bulletStartPos.y, bulletEndPos.y);
			}
		}

		const float windowWidth = static_cast<float>(_windowSize.x);
		const float windowHeight = static_cast<float>(_windowSize.y);

		player->SetPos({windowWidth - _tankSize, windowHeight - _tankSize});
		{
			//success shot up test, try to create inside screen bullet
			player->SetDirection(UP);
			player->Shot();
			if (const auto bullet = dynamic_cast<Bullet*>(allPawns.back().get()))
			{
				const FPoint bulletStartPos = bullet->GetPos();
				bullet->Move(deltaTime);
				const FPoint bulletEndPos = bullet->GetPos();
				EXPECT_GT(bulletStartPos.y, bulletEndPos.y);
				EXPECT_EQ(bulletStartPos.x, bulletEndPos.x);
			}
		}
		{
			//success move left test, try to move inside screen bullet
			player->SetDirection(LEFT);
			player->Shot();
			if (const auto bullet = dynamic_cast<Bullet*>(allPawns.back().get()))
			{
				const FPoint bulletStartPos = bullet->GetPos();
				bullet->Move(deltaTime);
				const FPoint bulletEndPos = bullet->GetPos();
				EXPECT_GT(bulletStartPos.x, bulletEndPos.x);
				EXPECT_EQ(bulletStartPos.y, bulletEndPos.y);
			}
		}
	}
}

// Check that tank can shoot inside of screen and can't outside
TEST_F(PlayerTest, BulletMoveOutSide)
{
	if (const auto player = dynamic_cast<MockPlayerOne*>(allPawns.back().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;
		const float windowWidth = static_cast<float>(_windowSize.x);
		const float windowHeight = static_cast<float>(_windowSize.y);

		player->SetPos({0.f, 0.f});
		{
			//fail bullet move down test, try to move outside screen bullet
			player->SetDirection(DOWN);
			player->Shot();
			if (const auto bullet = dynamic_cast<Bullet*>(allPawns.back().get()))
			{
				const float bulletWidth = player->GetBulletWidth();
				const float bulletHeight = player->GetBulletHeight();
				bullet->SetPos({windowWidth - bulletWidth, windowHeight - bulletHeight});
				const FPoint bulletStart = bullet->GetPos();
				bullet->Move(deltaTime);
				EXPECT_EQ(bulletStart, bullet->GetPos());
			}
		}
		{
			//fail bullet move right test, try to move outside screen bullet
			player->SetDirection(RIGHT);
			player->Shot();
			if (const auto bullet = dynamic_cast<Bullet*>(allPawns.back().get()))
			{
				const float bulletWidth = player->GetBulletWidth();
				const float bulletHeight = player->GetBulletHeight();
				bullet->SetPos({windowWidth - bulletWidth, windowHeight - bulletHeight});
				const FPoint bulletStartPos = bullet->GetPos();
				bullet->Move(deltaTime);
				EXPECT_EQ(bulletStartPos, bullet->GetPos());
			}
		}

		player->SetPos({windowWidth - _tankSize, windowHeight - _tankSize});
		{
			//fail bullet move up test, try to move outside screen bullet
			player->SetDirection(UP);
			player->Shot();
			if (const auto bullet = dynamic_cast<Bullet*>(allPawns.back().get()))
			{
				bullet->SetPos({0.f, 0.f});
				const FPoint bulletStart = bullet->GetPos();
				bullet->Move(deltaTime);
				EXPECT_EQ(bulletStart, bullet->GetPos());
			}
		}
		{
			//fail bullet move left test, try to move outside screen bullet
			player->SetDirection(LEFT);
			player->Shot();
			if (const auto bullet = dynamic_cast<Bullet*>(allPawns.back().get()))
			{
				bullet->SetPos({0.f, 0.f});
				const FPoint bulletStart = bullet->GetPos();
				bullet->Move(deltaTime);
				EXPECT_EQ(bulletStart, bullet->GetPos());
			}
		}
	}
}

// Check that tank can "possibly" deal damage to them self
TEST_F(PlayerTest, BulletDamage)
{
	if (const auto player = dynamic_cast<MockPlayerOne*>(allPawns.back().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;

		player->SetPos({0.f, 0.f});
		{
			player->SetDirection(DOWN);
			player->Shot();
			if (const auto bullet = dynamic_cast<Bullet*>(allPawns.back().get()))
			{
				const auto health = player->GetHealth();
				auto playerPos = player->GetPos();
				player->SetPos({playerPos.x, player->GetBottomSide() + player->GetBulletHeight()});
				bullet->Move(deltaTime);
				EXPECT_GT(health, player->GetHealth());
			}
		}
		{
			player->SetDirection(RIGHT);
			player->Shot();
			if (const auto bullet = dynamic_cast<Bullet*>(allPawns.back().get()))
			{
				const auto health = player->GetHealth();
				auto playerPos = player->GetPos();
				player->SetPos({player->GetRightSide() + player->GetBulletWidth(), playerPos.y});
				bullet->Move(deltaTime);
				EXPECT_GT(health, player->GetHealth());
			}
		}

		player->SetPos({static_cast<float>(_windowSize.x) - _tankSize, static_cast<float>(_windowSize.y) - _tankSize});
		{
			player->SetDirection(UP);
			player->Shot();
			if (const auto bullet = dynamic_cast<Bullet*>(allPawns.back().get()))
			{
				const auto health = player->GetHealth();
				auto playerPos = player->GetPos();
				player->SetPos({playerPos.x, player->GetY() - player->GetHeight() - player->GetBulletHeight()});
				bullet->Move(deltaTime);
				EXPECT_GT(health, player->GetHealth());
			}
		}
		{
			player->SetDirection(LEFT);
			player->Shot();
			if (const auto bullet = dynamic_cast<Bullet*>(allPawns.back().get()))
			{
				const auto health = player->GetHealth();
				auto playerPos = player->GetPos();
				player->SetPos({player->GetX() - player->GetBulletWidth() - player->GetBulletWidth(), playerPos.y});
				bullet->Move(deltaTime);
				EXPECT_GT(health, player->GetHealth());
			}
		}
	}

	//TODO: write collision test between:
	//two tanks,
	//two bullets,
	//tank and obstacle,
	//bullet and obstacle
	//bullet and tank
}