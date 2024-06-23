#include "MockPlayerOne.h"
#include "../headers/Brick.h"
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
	float _bulletSpeed{300.f};
	int* _windowBuffer{nullptr};
	int _tankHealth = 100;

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
		_tankSize = gridSize * 3;// for better turns
		const Rectangle playerRect{0, 0, _tankSize, _tankSize};
		constexpr int tankColor = 0xeaea00;
		allPawns.emplace_back(std::make_shared<MockPlayerOne>(playerRect, tankColor, _tankSpeed, _tankHealth,
		                                                      _windowBuffer, _windowSize, &allPawns, _events));
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
			const float windowHeight = static_cast<float>(_windowSize.y);
			player->SetPos({0.f, windowHeight - _tankSize});
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
			const float windowWidth = static_cast<float>(_windowSize.x);
			player->SetPos({windowWidth - _tankSize, 0.f});
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

		{
			//moveUp Down
			player->SetPos({0.f, 0.f});
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
			player->SetPos({0.f, 0.f});
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
		}
		{
			//fail  moveUp Left
			player->SetPos({0.f, 0.f});
			const FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->MockMoveKeyPressed(LEFT);
			player->Move(deltaTime);
			EXPECT_EQ(startPos, player->GetPos());
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
		}
		{
			//fail moveUp Right
			player->SetPos({windowWidth - _tankSize, windowHeight - _tankSize});
			const FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->MockMoveKeyPressed(RIGHT);
			player->Move(deltaTime);
			EXPECT_EQ(startPos, player->GetPos());
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

// Check that tank cant move through tank
TEST_F(PlayerTest, TankCantPassThroughTank)
{
	constexpr float deltaTime = 1.f / 60.f;
	if (const auto player = dynamic_cast<MockPlayerOne*>(allPawns.back().get()))
	{
		const Rectangle playerRect2{0, _tankSize + 1, _tankSize, _tankSize};
		allPawns.emplace_back(std::make_shared<MockPlayerOne>(playerRect2, 0xeaea00, _tankSpeed, _tankHealth,
															  _windowBuffer, _windowSize, &allPawns, _events));

		if (const auto player2 = dynamic_cast<MockPlayerOne*>(allPawns.back().get()))
		{
			//both player should failure, because they face each other blocking move each other
			const FPoint playerStartPos = player->GetPos();
			player->MockResetKeyPressed();
			player->MockMoveKeyPressed(DOWN);
			player->Move(deltaTime);
			const FPoint playerEndPos = player->GetPos();
			EXPECT_EQ(playerStartPos.y, playerEndPos.y);
			EXPECT_EQ(playerStartPos.x, playerEndPos.x);

			const FPoint player2StartPos = player2->GetPos();
			player2->MockResetKeyPressed();
			player2->MockMoveKeyPressed(UP);
			player2->Move(deltaTime);
			const FPoint player2EndPos = player2->GetPos();
			EXPECT_EQ(player2StartPos.y, player2EndPos.y);
			EXPECT_EQ(player2StartPos.x, player2EndPos.x);
		}
	}
}

// Check that tank cant move through brick
TEST_F(PlayerTest, TankCantPassThroughBrick)
{
	if (const auto player = dynamic_cast<MockPlayerOne*>(allPawns.back().get()))
	{
		const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
		const Rectangle rect{0.f, _tankSize + 1, gridSize, gridSize};
		allPawns.emplace_back(std::make_shared<Brick>(rect, _windowBuffer, _windowSize, _events));

		if (const auto brick = dynamic_cast<Brick*>(allPawns.back().get()))
		{
			//moveDown player should failure, because below we have brick obstacle
			const FPoint startPos = player->GetPos();
			player->MockResetKeyPressed();
			player->MockMoveKeyPressed(DOWN);
			constexpr float deltaTime = 1.f / 60.f;
			player->Move(deltaTime);
			const FPoint endPos = player->GetPos();
			EXPECT_EQ(startPos.y, endPos.y);
			EXPECT_EQ(startPos.x, endPos.x);
		}
	}
}