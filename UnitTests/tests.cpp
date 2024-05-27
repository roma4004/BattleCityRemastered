// #include "../headers/Point.h"
#include "../headers/Bullet.h"
#include "MockPlayerOne.h"

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
	size_t _windowWidth{800};
	size_t _windowHeight{600};
	float _tankSize{};
	float _tankSpeed = 142;

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		constexpr int tankHealth = 100;
		const float gridSize = static_cast<float>(_windowHeight) / 50.f;
		_tankSize = gridSize * 3;// for better turns
		int* windowBuffer = nullptr;
		Rectangle playerRect{0, 0, _tankSize, _tankSize};
		allPawns.emplace_back(std::make_shared<MockPlayerOne>(playerRect, 0xeaea00, _tankSpeed, tankHealth,
															  windowBuffer, _windowWidth, _windowHeight, &allPawns,
															  _events));
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

		{//moveUp test
			player->SetPos({0.f, _tankSize + _tankSpeed});
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->MockMoveKeyPressed(UP);
			player->Move(deltaTime);
			FPoint endPos = player->GetPos();
			EXPECT_NE(startPos.y, endPos.y);
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_GT(startPos.y, endPos.y);
			// EXPECT_CALL(*player, Move(1.f)).Times(1);
		}

		{//moveUp Left
			player->SetPos({_tankSize + _tankSpeed, 0.f});
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->MockMoveKeyPressed(LEFT);
			player->Move(deltaTime);
			FPoint endPos = player->GetPos();
			EXPECT_NE(startPos.x, endPos.x);
			EXPECT_EQ(startPos.y, endPos.y);
			EXPECT_GT(startPos.x, endPos.x);
			// EXPECT_CALL(*player, Move(1.f)).Times(1);
		}

		const float windowWidth = static_cast<float>(_windowWidth);
		const float windowHeight = static_cast<float>(_windowHeight);

		{//moveUp Down
			player->SetPos({windowWidth - 1 - _tankSize, windowHeight - 1 - _tankSize - _tankSpeed});
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->MockMoveKeyPressed(DOWN);
			player->Move(deltaTime);
			FPoint endPos = player->GetPos();
			EXPECT_NE(startPos.y, endPos.y);
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_LT(startPos.y, endPos.y);
			// EXPECT_CALL(*player, Move(1.f)).Times(1);
		}

		{//moveUp Right
			player->SetPos({windowWidth - 1 - _tankSize - _tankSpeed, windowHeight - 1 - _tankSize});
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->MockMoveKeyPressed(RIGHT);
			player->Move(deltaTime);
			FPoint endPos = player->GetPos();
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

		{//fail moveUp test
			player->SetPos({0.f, 0.f});
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->MockMoveKeyPressed(UP);
			player->Move(deltaTime);
			FPoint endPos = player->GetPos();
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_EQ(startPos.y, endPos.y);
			// EXPECT_CALL(*player, Move(1.f)).Times(1);
		}

		{//fail  moveUp Left
			player->SetPos({0.f, 0.f});
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->MockMoveKeyPressed(LEFT);
			player->Move(deltaTime);
			FPoint endPos = player->GetPos();
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_EQ(startPos.y, endPos.y);
			// EXPECT_CALL(*player, Move(1.f)).Times(1);
		}

		const float windowWidth = static_cast<float>(_windowWidth);
		const float windowHeight = static_cast<float>(_windowHeight);

		{//fail moveUp Down
			player->SetPos({windowWidth - 1 - _tankSize, windowHeight - 1 - _tankSize});
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->MockMoveKeyPressed(DOWN);
			player->Move(deltaTime);
			FPoint endPos = player->GetPos();
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_EQ(startPos.y, endPos.y);
			// EXPECT_CALL(*player, Move(1.f)).Times(1);
		}

		{//fail moveUp Right
			player->SetPos({windowWidth - 1 - _tankSize, windowHeight - 1 - _tankSize});
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->MockMoveKeyPressed(RIGHT);
			player->Move(deltaTime);
			FPoint endPos = player->GetPos();
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_EQ(startPos.y, endPos.y);
			// EXPECT_CALL(*player, Move(1.f)).Times(1);
		}
	}
}

// Check that tank can shoot inside of screen
TEST_F(PlayerTest, TankShotInSideScreen)
{
	if (const auto player = dynamic_cast<MockPlayerOne*>(allPawns.back().get()))
	{
		{//success shot down test, try to create inside screen bullet
			size_t size = allPawns.size();
			player->SetPos({0.f, 0.f});
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->SetDirection(DOWN);
			player->MockShotKeyPressed();
			player->Shot();
			size_t size2 = allPawns.size();
			FPoint endPos = player->GetPos();
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_EQ(startPos.y, endPos.y);
			EXPECT_LT(size, size2);
		}

		{//success shot right test, try to create inside screen bullet
			size_t size = allPawns.size();
			player->SetPos({0.f, 0.f});
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->SetDirection(RIGHT);
			player->MockShotKeyPressed();
			player->Shot();
			size_t size2 = allPawns.size();
			FPoint endPos = player->GetPos();
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_EQ(startPos.y, endPos.y);
			EXPECT_LT(size, size2);
		}

		float windowWidth = static_cast<float>(_windowWidth);
		float windowHeight = static_cast<float>(_windowHeight);

		{//success shot up test, try to create inside screen bullet
			player->SetPos({windowWidth - 1 - _tankSize, windowHeight - 1 - _tankSize});
			size_t size = allPawns.size();
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->SetDirection(UP);
			player->MockShotKeyPressed();
			player->Shot();
			size_t size2 = allPawns.size();
			FPoint endPos = player->GetPos();
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_EQ(startPos.y, endPos.y);
			EXPECT_LT(size, size2);
		}

		{//success shot left test, try to create inside screen bullet
			player->SetPos({windowWidth - 1 - _tankSize, windowHeight - 1 - _tankSize});
			size_t size = allPawns.size();
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->SetDirection(LEFT);
			player->MockShotKeyPressed();
			player->Shot();
			size_t size2 = allPawns.size();
			FPoint endPos = player->GetPos();
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_EQ(startPos.y, endPos.y);
			EXPECT_LT(size, size2);
		}
	}
}

// Check that tank can't shoot outside of screen
TEST_F(PlayerTest, TankShotOutSideScreen)
{
	if (const auto player = dynamic_cast<MockPlayerOne*>(allPawns.back().get()))
	{
		{//fail shot up test, try to create outside screen bullet
			size_t size = allPawns.size();
			player->SetPos({0.f, 0.f});
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->SetDirection(UP);
			player->MockShotKeyPressed();
			player->Shot();
			size_t size2 = allPawns.size();
			FPoint endPos = player->GetPos();
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_EQ(startPos.y, endPos.y);
			EXPECT_EQ(size, size2);
		}

		{//fail shot left test, try to create outside screen bullet
			size_t size = allPawns.size();
			player->SetPos({0.f, 0.f});
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->SetDirection(LEFT);
			player->MockShotKeyPressed();
			player->Shot();
			size_t size2 = allPawns.size();
			FPoint endPos = player->GetPos();
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_EQ(startPos.y, endPos.y);
			EXPECT_EQ(size, size2);
		}

		const float windowWidth = static_cast<float>(_windowWidth);
		const float windowHeight = static_cast<float>(_windowHeight);

		{//fail shot down test, try to create outside screen bullet
			player->SetPos({windowWidth - 1 - _tankSize, windowHeight - 1 - _tankSize});
			size_t size = allPawns.size();
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->SetDirection(DOWN);
			player->MockShotKeyPressed();
			player->Shot();
			size_t size2 = allPawns.size();
			FPoint endPos = player->GetPos();
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_EQ(startPos.y, endPos.y);
			EXPECT_EQ(size, size2);
		}

		{//fail shot right test, try to create outside screen bullet
			player->SetPos({windowWidth - 1 - _tankSize, windowHeight - 1 - _tankSize});
			size_t size = allPawns.size();
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->SetDirection(RIGHT);
			player->MockShotKeyPressed();
			player->Shot();
			size_t size2 = allPawns.size();
			FPoint endPos = player->GetPos();
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_EQ(startPos.y, endPos.y);
			EXPECT_EQ(size, size2);
		}
	}
}

// Check that tank can shoot inside of screen and can't outside
TEST_F(PlayerTest, BulletMoveInsideScreen)
{
	if (const auto player = dynamic_cast<MockPlayerOne*>(allPawns.back().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;

		{//success bullet move down test, try to move inside screen bullet
			size_t size = allPawns.size();
			player->SetPos({0.f, 0.f});
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->SetDirection(DOWN);
			player->MockShotKeyPressed();
			player->Shot();
			size_t size2 = allPawns.size();
			FPoint endPos = player->GetPos();
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_EQ(startPos.y, endPos.y);
			EXPECT_LT(size, size2);
			if (auto bullet = dynamic_cast<Bullet*>(allPawns.back().get()))
			{
				FPoint bulletStartPos = bullet->GetPos();
				bullet->Move(deltaTime);
				FPoint bulletEndPos = bullet->GetPos();
				EXPECT_LT(bulletStartPos.y, bulletEndPos.y);
				EXPECT_EQ(bulletStartPos.x, bulletEndPos.x);
			}
		}

		const float windowWidth = static_cast<float>(_windowWidth);
		const float windowHeight = static_cast<float>(_windowHeight);

		{//success bullet right test, try to move inside screen bullet
			size_t size = allPawns.size();
			player->SetPos({0.f, 0.f});
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->SetDirection(RIGHT);
			player->MockShotKeyPressed();
			player->Shot();
			size_t size2 = allPawns.size();
			FPoint endPos = player->GetPos();
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_EQ(startPos.y, endPos.y);
			EXPECT_LT(size, size2);
			if (auto bullet = dynamic_cast<Bullet*>(allPawns.back().get()))
			{
				FPoint bulletStartPos = bullet->GetPos();
				bullet->Move(deltaTime);
				FPoint bulletEndPos = bullet->GetPos();
				EXPECT_LT(bulletStartPos.x, bulletEndPos.x);
				EXPECT_EQ(bulletStartPos.y, bulletEndPos.y);
			}
		}

		{//success shot up test, try to create inside screen bullet
			player->SetPos({windowWidth - 1 - _tankSize, windowHeight - 1 - _tankSize});
			size_t size = allPawns.size();
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->SetDirection(UP);
			player->MockShotKeyPressed();
			player->Shot();
			size_t size2 = allPawns.size();
			FPoint endPos = player->GetPos();
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_EQ(startPos.y, endPos.y);
			EXPECT_LT(size, size2);
			if (auto bullet = dynamic_cast<Bullet*>(allPawns.back().get()))
			{
				FPoint bulletStartPos = bullet->GetPos();
				bullet->Move(deltaTime);
				FPoint bulletEndPos = bullet->GetPos();
				EXPECT_GT(bulletStartPos.y, bulletEndPos.y);
				EXPECT_EQ(bulletStartPos.x, bulletEndPos.x);
			}
		}

		{//success move left test, try to move inside screen bullet
			player->SetPos({windowWidth - 1 - _tankSize, windowHeight - 1 - _tankSize});
			size_t size = allPawns.size();
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->SetDirection(LEFT);
			player->MockShotKeyPressed();
			player->Shot();
			size_t size2 = allPawns.size();
			FPoint endPos = player->GetPos();
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_EQ(startPos.y, endPos.y);
			EXPECT_LT(size, size2);
			if (auto bullet = dynamic_cast<Bullet*>(allPawns.back().get()))
			{
				FPoint bulletStartPos = bullet->GetPos();
				bullet->Move(deltaTime);
				FPoint bulletEndPos = bullet->GetPos();
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
		const float windowWidth = static_cast<float>(_windowWidth);
		const float windowHeight = static_cast<float>(_windowHeight);

		{//fail bullet move down test, try to move outside screen bullet
			size_t size = allPawns.size();
			player->SetPos({0.f, 0.f});
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->SetDirection(DOWN);
			player->MockShotKeyPressed();
			player->Shot();
			size_t size2 = allPawns.size();
			FPoint endPos = player->GetPos();
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_EQ(startPos.y, endPos.y);
			EXPECT_LT(size, size2);
			if (auto bullet = dynamic_cast<Bullet*>(allPawns.back().get()))
			{
				const float bulletWidth = player->GetBulletWidth();
				const float bulletHeight = player->GetBulletHeight();
				bullet->SetPos({windowWidth - 1 - bulletWidth, windowHeight - 1 - bulletHeight});
				FPoint bulletStart = bullet->GetPos();
				bullet->Move(deltaTime);
				FPoint bulletEndPos = bullet->GetPos();
				EXPECT_EQ(bulletStart.y, bulletEndPos.y);
				EXPECT_EQ(bulletStart.x, bulletEndPos.x);
			}
		}

		{//fail bullet move right test, try to move outside screen bullet
			size_t size = allPawns.size();
			player->SetPos({0.f, 0.f});
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->SetDirection(RIGHT);
			player->MockShotKeyPressed();
			player->Shot();
			size_t size2 = allPawns.size();
			FPoint endPos = player->GetPos();
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_EQ(startPos.y, endPos.y);
			EXPECT_LT(size, size2);
			if (auto bullet = dynamic_cast<Bullet*>(allPawns.back().get()))
			{
				const float bulletWidth = player->GetBulletWidth();
				const float bulletHeight = player->GetBulletHeight();
				bullet->SetPos({windowWidth - 1 - bulletWidth, windowHeight - 1 - bulletHeight});
				FPoint bulletStartPos = bullet->GetPos();
				bullet->Move(deltaTime);
				FPoint bulletEndPos = bullet->GetPos();
				EXPECT_EQ(bulletStartPos.y, bulletEndPos.y);
				EXPECT_EQ(bulletStartPos.x, bulletEndPos.x);
			}
		}

		{//fail bullet move up test, try to move outside screen bullet
			player->SetPos({windowWidth - 1 - _tankSize, windowHeight - 1 - _tankSize});
			size_t size = allPawns.size();
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->SetDirection(UP);
			player->MockShotKeyPressed();
			player->Shot();
			size_t size2 = allPawns.size();
			FPoint endPos = player->GetPos();
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_EQ(startPos.y, endPos.y);
			EXPECT_LT(size, size2);
			if (auto bullet = dynamic_cast<Bullet*>(allPawns.back().get()))
			{
				bullet->SetPos({0.f, 0.f});
				FPoint bulletStartPos = bullet->GetPos();
				bullet->Move(deltaTime);
				FPoint bulletEndPos = bullet->GetPos();
				EXPECT_EQ(bulletStartPos.x, bulletEndPos.x);
				EXPECT_EQ(bulletStartPos.y, bulletEndPos.y);
			}
		}

		{//fail bullet move left test, try to move outside screen bullet
			player->SetPos({windowWidth - 1 - _tankSize, windowHeight - 1 - _tankSize});
			size_t size = allPawns.size();
			FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->SetDirection(LEFT);
			player->MockShotKeyPressed();
			player->Shot();
			size_t size2 = allPawns.size();
			FPoint endPos = player->GetPos();
			EXPECT_EQ(startPos.x, endPos.x);
			EXPECT_EQ(startPos.y, endPos.y);
			EXPECT_LT(size, size2);
			if (auto bullet = dynamic_cast<Bullet*>(allPawns.back().get()))
			{
				bullet->SetPos({0.f, 0.f});
				FPoint bulletStartPos = bullet->GetPos();
				bullet->Move(deltaTime);
				FPoint bulletEndPos = bullet->GetPos();
				EXPECT_EQ(bulletStartPos.x, bulletEndPos.x);
				EXPECT_EQ(bulletStartPos.y, bulletEndPos.y);
			}
		}
	}
}
