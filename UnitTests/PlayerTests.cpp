#include "../headers/Brick.h"
#include "../headers/EventSystem.h"
#include "../headers/InputProviderForPlayerOne.h"
#include "../headers/Iron.h"
#include "../headers/PlayerOne.h"
#include "../headers/PlayerTwo.h"
#include "../headers/Water.h"

#include "gtest/gtest.h"

#include <memory>

class PlayerTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events;
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	UPoint _windowSize{800, 600};
	float _tankSize{};
	float _tankSpeed{142};
	float _bulletSpeed{300.f};
	int* _windowBuffer{nullptr};
	int _tankHealth = 100;
	std::shared_ptr<BulletPool> _bulletPool;

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
		_tankSize = gridSize * 3;// for better turns
		const Rectangle rect{0, 0, _tankSize, _tankSize};
		constexpr int yellow = 0xeaea00;
		std::string name = "PlayerOne";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		//TODO: initialize with btn names
		_bulletPool = std::make_shared<BulletPool>();
		_allObjects.reserve(4);
		_allObjects.emplace_back(std::make_shared<PlayerOne>(rect, yellow, _tankHealth, _windowBuffer, _windowSize, UP,
		                                                     _tankSpeed, &_allObjects, _events, name, fraction,
		                                                     inputProvider, _bulletPool));
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

// Check that tank can move inside screen
TEST_F(PlayerTest, TankMoveInSideScreenUp)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;
		const float windowHeight = static_cast<float>(_windowSize.y);
		player->SetPos({0.f, windowHeight - _tankSize});
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("W_Pressed");
		_events->EmitEvent<const float>("TickUpdate", deltaTime);

		const FPoint endPos = player->GetPos();
		EXPECT_NE(startPos.y, endPos.y);
		EXPECT_EQ(startPos.x, endPos.x);
		EXPECT_GT(startPos.y, endPos.y);
		// EXPECT_CALL(*player, Move(1.f)).Times(1);

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank can move inside screen
TEST_F(PlayerTest, TankMoveInSideScreenLeft)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;
		const float windowWidth = static_cast<float>(_windowSize.x);
		player->SetPos({windowWidth - _tankSize, 0.f});
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("A_Pressed");
		_events->EmitEvent<const float>("TickUpdate", deltaTime);

		const FPoint endPos = player->GetPos();
		EXPECT_NE(startPos.x, endPos.x);
		EXPECT_EQ(startPos.y, endPos.y);
		EXPECT_GT(startPos.x, endPos.x);
		// EXPECT_CALL(*player, Move(1.f)).Times(1);

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank can move inside screen
TEST_F(PlayerTest, TankMoveInSideScreenDown)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;
		player->SetPos({0.f, 0.f});
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("S_Pressed");
		_events->EmitEvent<const float>("TickUpdate", deltaTime);

		const FPoint endPos = player->GetPos();
		EXPECT_NE(startPos.y, endPos.y);
		EXPECT_EQ(startPos.x, endPos.x);
		EXPECT_LT(startPos.y, endPos.y);
		// EXPECT_CALL(*player, Move(1.f)).Times(1);

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank can move inside screen
TEST_F(PlayerTest, TankMoveInSideScreenRight)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;
		player->SetPos({0.f, 0.f});
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("D_Pressed");
		_events->EmitEvent<const float>("TickUpdate", deltaTime);

		const FPoint endPos = player->GetPos();
		EXPECT_NE(startPos.x, endPos.x);
		EXPECT_EQ(startPos.y, endPos.y);
		EXPECT_LT(startPos.x, endPos.x);
		// EXPECT_CALL(*player, Move(1.f)).Times(1);

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank cannot move out of screen
TEST_F(PlayerTest, TankMoveOutSideScreenUp)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;

		player->SetPos({0.f, 0.f});
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("W_Pressed");
		_events->EmitEvent<const float>("TickUpdate", deltaTime);

		EXPECT_EQ(startPos, player->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank cannot move out of screen
TEST_F(PlayerTest, TankMoveOutSideScreenLeft)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;

		player->SetPos({0.f, 0.f});
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("A_Pressed");
		_events->EmitEvent<const float>("TickUpdate", deltaTime);

		EXPECT_EQ(startPos, player->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank cannot move out of screen
TEST_F(PlayerTest, TankMoveOutSideScreenDown)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;

		const float windowWidth = static_cast<float>(_windowSize.x);
		const float windowHeight = static_cast<float>(_windowSize.y);
		player->SetPos({windowWidth - _tankSize, windowHeight - _tankSize});
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("S_Pressed");
		_events->EmitEvent<const float>("TickUpdate", deltaTime);

		EXPECT_EQ(startPos, player->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank cannot move out of screen
TEST_F(PlayerTest, TankMoveOutSideScreenRight)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;

		const float windowWidth = static_cast<float>(_windowSize.x);
		const float windowHeight = static_cast<float>(_windowSize.y);
		player->SetPos({windowWidth - _tankSize, windowHeight - _tankSize});
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("D_Pressed");
		_events->EmitEvent<const float>("TickUpdate", deltaTime);

		EXPECT_EQ(startPos, player->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank set their position correctly
TEST_F(PlayerTest, TankSetPos)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		player->SetPos({});
		const FPoint startPos = player->GetPos();

		player->SetPos({static_cast<float>(_windowSize.x), static_cast<float>(_windowSize.y)});

		EXPECT_LT(startPos, player->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank don't move when shooting
TEST_F(PlayerTest, TankDontMoveWhenShotUp)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		player->SetPos({static_cast<float>(_windowSize.x) / 2.f, static_cast<float>(_windowSize.y) / 2.f});
		player->SetDirection(UP);
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("Space_Pressed");
		constexpr float deltaTime = 1.f / 60.f;
		_events->EmitEvent<const float>("TickUpdate", deltaTime);

		EXPECT_EQ(startPos, player->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank don't move when shooting
TEST_F(PlayerTest, TankDontMoveWhenShotLeft)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		player->SetPos({static_cast<float>(_windowSize.x) / 2.f, static_cast<float>(_windowSize.y) / 2.f});
		player->SetDirection(LEFT);
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("Space_Pressed");
		constexpr float deltaTime = 1.f / 60.f;
		_events->EmitEvent<const float>("TickUpdate", deltaTime);

		EXPECT_EQ(startPos, player->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank don't move when shooting
TEST_F(PlayerTest, TankDontMoveWhenShotDown)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		player->SetPos({static_cast<float>(_windowSize.x) / 2.f, static_cast<float>(_windowSize.y) / 2.f});
		player->SetDirection(DOWN);
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("Space_Pressed");
		constexpr float deltaTime = 1.f / 60.f;
		_events->EmitEvent<const float>("TickUpdate", deltaTime);

		EXPECT_EQ(startPos, player->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank don't move when shooting
TEST_F(PlayerTest, TankDontMoveWhenShotRight)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		player->SetPos({static_cast<float>(_windowSize.x) / 2.f, static_cast<float>(_windowSize.y) / 2.f});
		player->SetDirection(RIGHT);
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("Space_Pressed");
		constexpr float deltaTime = 1.f / 60.f;
		_events->EmitEvent<const float>("TickUpdate", deltaTime);

		EXPECT_EQ(startPos, player->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank can shoot inside of screen
TEST_F(PlayerTest, TankShotInSideScreenDown)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		player->SetPos({0.f, 0.f});
		//success shot down test, try to create inside screen bullet
		player->SetDirection(DOWN);
		const size_t size = _allObjects.size();

		_events->EmitEvent("Space_Pressed");
		constexpr float deltaTime = 1.f / 60.f;
		_events->EmitEvent<const float>("TickUpdate", deltaTime);

		EXPECT_LT(size, _allObjects.size());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank can shoot inside of screen
TEST_F(PlayerTest, TankShotInSideScreenRight)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		player->SetPos({0.f, 0.f});
		//success shot right test, try to create inside screen bullet
		const size_t size = _allObjects.size();

		_events->EmitEvent("D_Pressed");
		_events->EmitEvent("Space_Pressed");
		constexpr float deltaTime = 1.f / 60.f;
		_events->EmitEvent<const float>("TickUpdate", deltaTime);

		EXPECT_LT(size, _allObjects.size());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank can shoot inside of screen
TEST_F(PlayerTest, TankShotInSideScreenUp)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		player->SetPos({static_cast<float>(_windowSize.x) - _tankSize, static_cast<float>(_windowSize.y) - _tankSize});
		//success shot up test, try to create inside screen bullet
		const size_t size = _allObjects.size();

		_events->EmitEvent("W_Pressed");
		_events->EmitEvent("Space_Pressed");
		constexpr float deltaTime = 1.f / 60.f;
		_events->EmitEvent<const float>("TickUpdate", deltaTime);

		EXPECT_LT(size, _allObjects.size());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank can shoot inside of screen
TEST_F(PlayerTest, TankShotInSideScreenLeft)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		player->SetPos({static_cast<float>(_windowSize.x) - _tankSize, static_cast<float>(_windowSize.y) - _tankSize});
		//success shot left test, try to create inside screen bullet
		const size_t size = _allObjects.size();

		_events->EmitEvent("A_Pressed");
		_events->EmitEvent("Space_Pressed");
		constexpr float deltaTime = 1.f / 60.f;
		_events->EmitEvent<const float>("TickUpdate", deltaTime);

		EXPECT_LT(size, _allObjects.size());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank can't shoot outside of screen
TEST_F(PlayerTest, TankShotOutSideScreen)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		player->SetPos({0.f, 0.f});
		{
			//fail shot up test, try to create outside screen bullet
			const size_t size = _allObjects.size();

			_events->EmitEvent("W_Pressed");
			_events->EmitEvent("Space_Pressed");
			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(size, _allObjects.size());
		}
		{
			//fail shot left test, try to create outside screen bullet
			const size_t size = _allObjects.size();

			_events->EmitEvent("A_Pressed");
			_events->EmitEvent("Space_Pressed");
			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(size, _allObjects.size());
		}

		player->SetPos({static_cast<float>(_windowSize.x) - _tankSize, static_cast<float>(_windowSize.y) - _tankSize});
		{
			//fail shot down test, try to create outside screen bullet
			const size_t size = _allObjects.size();

			_events->EmitEvent("S_Pressed");
			_events->EmitEvent("Space_Pressed");
			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(size, _allObjects.size());
		}
		{
			//fail shot right test, try to create outside screen bullet
			const size_t size = _allObjects.size();

			_events->EmitEvent("D_Pressed");
			_events->EmitEvent("Space_Pressed");
			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(size, _allObjects.size());

			return;
		}
	}

	EXPECT_TRUE(false);
}

// Check that tank cant move through tank
TEST_F(PlayerTest, TankCantPassThroughTank)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		const Rectangle rect{0, _tankSize + 1, _tankSize, _tankSize};
		constexpr int green = 0x408000;
		std::string name = "PlayerTwo";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		_allObjects.emplace_back(std::make_shared<PlayerTwo>(rect, green, _tankHealth, _windowBuffer, _windowSize, UP,
		                                                     _tankSpeed, &_allObjects, _events, name, fraction,
		                                                     inputProvider, _bulletPool));

		if (const auto player2 = dynamic_cast<PlayerTwo*>(_allObjects.back().get()))
		{
			//both player should failure, because they face each other blocking move each other
			const FPoint playerStartPos = player->GetPos();
			const FPoint player2StartPos = player2->GetPos();

			_events->EmitEvent("A_Pressed");
			_events->EmitEvent("ArrowUp_Pressed");
			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(playerStartPos, player->GetPos());
			EXPECT_EQ(player2StartPos, player2->GetPos());

			return;
		}
	}

	EXPECT_TRUE(false);
}

// Check that tank cant move through brick
TEST_F(PlayerTest, TankCantPassThroughBrick)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
		const Rectangle rect{0.f, _tankSize + 1, gridSize, gridSize};
		_allObjects.emplace_back(std::make_shared<Brick>(rect, _windowBuffer, _windowSize, _events));

		if (const auto brick = dynamic_cast<Brick*>(_allObjects.back().get()))
		{
			//moveDown player should failure, because below we have brick obstacle
			const FPoint startPos = player->GetPos();

			_events->EmitEvent("W_Pressed");
			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(startPos, player->GetPos());

			return;
		}
	}

	EXPECT_TRUE(false);
}

// Check that tank cant move through iron
TEST_F(PlayerTest, TankCantPassThroughIron)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
		const Rectangle rect{0.f, _tankSize + 1, gridSize, gridSize};
		_allObjects.emplace_back(std::make_shared<Iron>(rect, _windowBuffer, _windowSize, _events));

		if (const auto brick = dynamic_cast<Iron*>(_allObjects.back().get()))
		{
			//moveDown player should failure, because below we have brick obstacle
			const FPoint startPos = player->GetPos();

			_events->EmitEvent("W_Pressed");
			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(startPos, player->GetPos());

			return;
		}
	}

	EXPECT_TRUE(false);
}

// Check that tank cant move through water
TEST_F(PlayerTest, TankCantPassThroughWater)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
		const Rectangle rect{0.f, _tankSize + 1, gridSize, gridSize};
		_allObjects.emplace_back(std::make_shared<Water>(rect, _windowBuffer, _windowSize, _events));

		if (const auto water = dynamic_cast<Water*>(_allObjects.back().get()))
		{
			//moveDown player should failure, because below we have Iron obstacle
			const FPoint startPos = player->GetPos();

			_events->EmitEvent("W_Pressed");
			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(startPos, player->GetPos());

			return;
		}
	}

	EXPECT_TRUE(false);
}
