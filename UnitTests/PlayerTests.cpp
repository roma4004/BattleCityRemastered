#include "../packages/sdl2.nuget.2.32.8/build/native/include/SDL_events.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/TankSpawner.h"
#include "components/input/InputProviderForPlayerOne.h"
#include "components/input/InputProviderForPlayerTwo.h"
#include "components/managers/BonusEffectManager.h"
#include "components/managers/StateManager.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/FortressWall.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/Player.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/TankType.h"
#include "components/managers/DelayedSpawnManager.h"
#include "gtest/gtest.h"
#include <memory>
#include <boost/uuid/random_generator.hpp>
class EventSystem;
class StateManager;
class TankSpawner;
class RespawnManager;

class PlayerTest : public testing::Test
{
	using buuid = boost::uuids::uuid;

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<StateManager> _stateManager{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<DelayedSpawnManager> _spawnDelayManager{nullptr};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	UPoint _windowSize{.x = 800, .y = 600};
	int _tankHealth{100};
	float _tankSize{};
	float _tankSpeed{142};
	float _bulletSpeed{300.f};
	float _gridSize{};
	double _deltaTimeOneFrame{1.f / 60.f};
	std::string _name = "Player1";
	std::string _name2 = "Player2";
	std::string _fraction = "PlayerTeam";
	buuid _uuid{};
	GameMode _gameMode{GameMode::OnePlayer};
	bool _isGameWon{false};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _windowSize, _gameMode);
		_stateManager = std::make_shared<StateManager>(_events);
		_tankSpawner = std::make_shared<TankSpawner>(_windowSize, &_allObjects, _events);
		_spawnDelayManager = std::make_shared<DelayedSpawnManager>(_events);
		_gridSize = static_cast<float>(_windowSize.y) / 50.f;
		_tankSize = _gridSize * 3;// for better turns

		constexpr int yellow{0xeaea00};
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(_events);

		const ObjRectangle rect{.x = 0, .y = 0, .w = _tankSize, .h = _tankSize};
		BaseObjProperty baseObjProperty{.rect = rect,
										.color = yellow,
										.health = _tankHealth,
										.uuid = _uuid,
										.name = _name,
										.fraction = _fraction};
		PawnProperty pawnProperty{
				.baseObjProperty = std::move(baseObjProperty),
				.allObjects = &_allObjects,
				.events = _events,
				.tier = 1,
				.speed = _tankSpeed,
				.windowSize = _windowSize,
				.dir = Direction::UP,
				.gameMode = _gameMode};
		constexpr bool enableByDefault{true};

		_allObjects.reserve(4);
		BonusEffectProperty bonusEffects{};
		_allObjects.emplace_back(
				std::make_shared<Player>(
						std::move(pawnProperty), _bulletPool, std::move(inputProvider), BonusEffectProperty{},
						enableByDefault));
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

// Check that tank can move inside the screen
TEST_F(PlayerTest, TankMoveInSideScreenUp)
{
	if (const auto player = dynamic_cast<Player*>(_allObjects.front().get()))
	{
		const auto windowHeight = static_cast<float>(_windowSize.y);
		player->SetPos({.x = 0.f, .y = windowHeight - _tankSize});
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("P1_Move_Up_Pressed");
		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		const FPoint endPos = player->GetPos();
		EXPECT_NE(startPos.y, endPos.y);
		EXPECT_EQ(startPos.x, endPos.x);
		EXPECT_GT(startPos.y, endPos.y);
		// EXPECT_CALL(*player, Move(1.f)).Times(1);

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank can move inside the screen
TEST_F(PlayerTest, TankMoveInSideScreenLeft)
{
	if (const auto player = dynamic_cast<Player*>(_allObjects.front().get()))
	{
		const auto windowWidth = static_cast<float>(_windowSize.x);
		player->SetPos({.x = windowWidth - _tankSize, .y = 0.f});
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("P1_Move_Left_Pressed");
		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		const FPoint endPos = player->GetPos();
		EXPECT_NE(startPos.x, endPos.x);
		EXPECT_EQ(startPos.y, endPos.y);
		EXPECT_GT(startPos.x, endPos.x);
		// EXPECT_CALL(*player, Move(1.f)).Times(1);

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank can move inside the screen
TEST_F(PlayerTest, TankMoveInSideScreenDown)
{
	if (const auto player = dynamic_cast<Player*>(_allObjects.front().get()))
	{
		player->SetPos({.x = 0.f, .y = 0.f});
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("P1_Move_Down_Pressed");
		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		const FPoint endPos = player->GetPos();
		EXPECT_NE(startPos.y, endPos.y);
		EXPECT_EQ(startPos.x, endPos.x);
		EXPECT_LT(startPos.y, endPos.y);
		// EXPECT_CALL(*player, Move(1.f)).Times(1);

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank can move inside the screen
TEST_F(PlayerTest, TankMoveInSideScreenRight)
{
	if (const auto player = dynamic_cast<Player*>(_allObjects.front().get()))
	{
		player->SetPos({.x = 0.f, .y = 0.f});
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("P1_Move_Right_Pressed");
		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

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
	if (const auto player = dynamic_cast<Player*>(_allObjects.front().get()))
	{
		player->SetPos({.x = 0.f, .y = 0.f});
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("P1_Move_Up_Pressed");
		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_EQ(startPos, player->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank cannot move out of screen
TEST_F(PlayerTest, TankMoveOutSideScreenLeft)
{
	if (const auto player = dynamic_cast<Player*>(_allObjects.front().get()))
	{
		player->SetPos({.x = 0.f, .y = 0.f});
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("P1_Move_Left_Pressed");
		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_EQ(startPos, player->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank cannot move out of screen
TEST_F(PlayerTest, TankMoveOutSideScreenDown)
{
	if (const auto player = dynamic_cast<Player*>(_allObjects.front().get()))
	{
		const auto windowWidth = static_cast<float>(_windowSize.x);
		const auto windowHeight = static_cast<float>(_windowSize.y);
		player->SetPos({.x = windowWidth - _tankSize, .y = windowHeight - _tankSize});
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("P1_Move_Down_Pressed");
		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_EQ(startPos, player->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank cannot move out of screen
TEST_F(PlayerTest, TankMoveOutSideScreenRight)
{
	if (const auto player = dynamic_cast<Player*>(_allObjects.front().get()))
	{
		const auto windowWidth = static_cast<float>(_windowSize.x);
		const auto windowHeight = static_cast<float>(_windowSize.y);
		player->SetPos({.x = windowWidth - _tankSize, .y = windowHeight - _tankSize});
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("P1_Move_Right_Pressed");
		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_EQ(startPos, player->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank set their position correctly
TEST_F(PlayerTest, TankSetPos)
{
	if (const auto player = dynamic_cast<Player*>(_allObjects.front().get()))
	{
		player->SetPos({});
		const FPoint startPos = player->GetPos();

		player->SetPos({.x = static_cast<float>(_windowSize.x), .y = static_cast<float>(_windowSize.y)});

		EXPECT_LT(startPos, player->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank don't move when shooting
TEST_F(PlayerTest, TankDontMoveWhenShotUp)
{
	if (const auto player = dynamic_cast<Player*>(_allObjects.front().get()))
	{
		player->SetPos({.x = static_cast<float>(_windowSize.x) / 2.f,
						.y = static_cast<float>(_windowSize.y) / 2.f});
		player->SetDirection(Direction::UP);
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("P1_Fire_Pressed");
		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_EQ(startPos, player->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank don't move when shooting
TEST_F(PlayerTest, TankDontMoveWhenShotLeft)
{
	if (const auto player = dynamic_cast<Player*>(_allObjects.front().get()))
	{
		player->SetPos({.x = static_cast<float>(_windowSize.x) / 2.f,
						.y = static_cast<float>(_windowSize.y) / 2.f});
		player->SetDirection(Direction::LEFT);
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("P1_Fire_Pressed");
		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_EQ(startPos, player->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank don't move when shooting
TEST_F(PlayerTest, TankDontMoveWhenShotDown)
{
	if (const auto player = dynamic_cast<Player*>(_allObjects.front().get()))
	{
		player->SetPos({.x = static_cast<float>(_windowSize.x) / 2.f,
						.y = static_cast<float>(_windowSize.y) / 2.f});
		player->SetDirection(Direction::DOWN);
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("P1_Fire_Pressed");
		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_EQ(startPos, player->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank don't move when shooting
TEST_F(PlayerTest, TankDontMoveWhenShotRight)
{
	if (const auto player = dynamic_cast<Player*>(_allObjects.front().get()))
	{
		player->SetPos({.x = static_cast<float>(_windowSize.x) / 2.f,
						.y = static_cast<float>(_windowSize.y) / 2.f});
		player->SetDirection(Direction::RIGHT);
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("P1_Fire_Pressed");
		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_EQ(startPos, player->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank can shoot inside the screen
TEST_F(PlayerTest, TankShotInSideScreenDown)
{
	if (const auto player = dynamic_cast<Player*>(_allObjects.front().get()))
	{
		player->SetPos({.x = 0.f, .y = 0.f});
		//success shot down test, try to create an inside screen bullet
		player->SetDirection(Direction::DOWN);
		const size_t size = _allObjects.size();

		_events->EmitEvent("P1_Fire_Pressed");
		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_LT(size, _allObjects.size());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank can shoot inside the screen
TEST_F(PlayerTest, TankShotInSideScreenRight)
{
	if (const auto player = dynamic_cast<Player*>(_allObjects.front().get()))
	{
		player->SetPos({.x = 0.f, .y = 0.f});
		//success shot right test, try to create an inside screen bullet
		const size_t size = _allObjects.size();

		_events->EmitEvent("P1_Move_Right_Pressed");
		_events->EmitEvent("P1_Fire_Pressed");
		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_LT(size, _allObjects.size());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank can shoot inside the screen
TEST_F(PlayerTest, TankShotInSideScreenUp)
{
	if (const auto player = dynamic_cast<Player*>(_allObjects.front().get()))
	{
		player->SetPos({.x = static_cast<float>(_windowSize.x) - _tankSize,
						.y = static_cast<float>(_windowSize.y) - _tankSize});
		//success shot up test, try to create an inside screen bullet
		const size_t size = _allObjects.size();

		_events->EmitEvent("P1_Move_Up_Pressed");
		_events->EmitEvent("P1_Fire_Pressed");
		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_LT(size, _allObjects.size());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank can shoot inside the screen
TEST_F(PlayerTest, TankShotInSideScreenLeft)
{
	if (const auto player = dynamic_cast<Player*>(_allObjects.front().get()))
	{
		player->SetPos({.x = static_cast<float>(_windowSize.x) - _tankSize,
						.y = static_cast<float>(_windowSize.y) - _tankSize});
		//success shot left test, try to create an inside screen bullet
		const size_t size = _allObjects.size();

		_events->EmitEvent("P1_Move_Left_Pressed");
		_events->EmitEvent("P1_Fire_Pressed");
		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_LT(size, _allObjects.size());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank can't shoot outside the screen
TEST_F(PlayerTest, TankShotOutSideScreen)
{
	if (const auto player = dynamic_cast<Player*>(_allObjects.front().get()))
	{
		player->SetPos({.x = 0.f, .y = 0.f});
		{
			//fail the shot up test, try to create an outside screen bullet
			const size_t size = _allObjects.size();

			_events->EmitEvent("P1_Move_Up_Pressed");
			_events->EmitEvent("P1_Fire_Pressed");
			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

			EXPECT_EQ(size, _allObjects.size());
		}
		{
			//fail shot left test, try to create an outside screen bullet
			const size_t size = _allObjects.size();

			_events->EmitEvent("P1_Move_Left_Pressed");
			_events->EmitEvent("P1_Fire_Pressed");
			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

			EXPECT_EQ(size, _allObjects.size());
		}

		player->SetPos({.x = static_cast<float>(_windowSize.x) - _tankSize,
						.y = static_cast<float>(_windowSize.y) - _tankSize});
		{
			//fail the shot down test, try to create an outside screen bullet
			const size_t size = _allObjects.size();

			_events->EmitEvent("P1_Move_Down_Pressed");
			_events->EmitEvent("P1_Fire_Pressed");
			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

			EXPECT_EQ(size, _allObjects.size());
		}
		{
			//fail the shot right test, try to create an outside screen bullet
			const size_t size = _allObjects.size();

			_events->EmitEvent("P1_Move_Right_Pressed");
			_events->EmitEvent("P1_Fire_Pressed");
			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

			EXPECT_EQ(size, _allObjects.size());

			return;
		}
	}

	EXPECT_TRUE(false);
}

// Check that tank can't move through tank
TEST_F(PlayerTest, TankCantPassThroughTank)
{
	if (const auto player = dynamic_cast<const Player*>(_allObjects.front().get()))
	{
		constexpr int green = 0x408000;
		std::unique_ptr<IInputProvider> inputProvider2 = std::make_unique<InputProviderForPlayerTwo>(_events);
		ObjRectangle rect{.x = 0, .y = _tankSize + 1, .w = _tankSize, .h = _tankSize};
		BaseObjProperty baseObjProperty{.rect = rect,
										.color = green,
										.health = _tankHealth,
										.uuid = _uuid,
										.name = _name,
										.fraction = _fraction};
		PawnProperty pawnProperty{
				.baseObjProperty = std::move(baseObjProperty),
				.allObjects = &_allObjects,
				.events = _events,
				.tier = 1,
				.speed = _tankSpeed,
				.windowSize = _windowSize,
				.dir = Direction::UP,
				.gameMode = _gameMode};
		_allObjects.emplace_back(
				std::make_shared<Player>(
						std::move(pawnProperty), _bulletPool, std::move(inputProvider2), BonusEffectProperty{}));

		if (const auto player2 = dynamic_cast<const Player*>(_allObjects.back().get()))
		{
			//both players should failure, because they face each other blocking move each other
			const FPoint playerStartPos = player->GetPos();
			const FPoint player2StartPos = player2->GetPos();

			_events->EmitEvent("P1_Move_Left_Pressed");
			_events->EmitEvent("P2_Move_Up_Pressed");
			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

			EXPECT_EQ(playerStartPos, player->GetPos());
			EXPECT_EQ(player2StartPos, player2->GetPos());

			return;
		}
	}

	EXPECT_TRUE(false);
}

// Check that tank can't move through brickWall
TEST_F(PlayerTest, TankCantPassThroughBrickWall)
{
	if (const auto player = dynamic_cast<const Player*>(_allObjects.front().get()))
	{
		_allObjects.emplace_back(
				std::make_shared<BrickWall>(
						ObjRectangle{.x = 0.f, .y = _tankSize + 1, .w = _gridSize, .h = _gridSize}, _events, _uuid,
						_gameMode));

		//moveDown player should failure, because below we have a brickWall obstacle
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("P1_Move_Down_Pressed");
		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_EQ(startPos, player->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank cant move through steelWall
TEST_F(PlayerTest, TankCantPassThroughSteelWall)
{
	if (const auto player = dynamic_cast<const Player*>(_allObjects.front().get()))
	{
		_allObjects.emplace_back(
				std::make_shared<SteelWall>(
						ObjRectangle{.x = 0.f, .y = _tankSize + 1, .w = _gridSize, .h = _gridSize}, _events, _uuid,
						_gameMode));

		//moveDown player should failure, because below we have a steelWall obstacle
		const FPoint startPos = player->GetPos();

		_events->EmitEvent("P1_Move_Down_Pressed");
		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_EQ(startPos, player->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank cant move through water
TEST_F(PlayerTest, TankCantPassThroughWater)
{
	if (const auto player = dynamic_cast<const Player*>(_allObjects.front().get()))
	{
		_allObjects.emplace_back(
				std::make_shared<WaterTile>(
						ObjRectangle{.x = 0.f, .y = _tankSize + 1, .w = _gridSize, .h = _gridSize},
						_events, _uuid, _gameMode));

		if (dynamic_cast<WaterTile*>(_allObjects.back().get()))
		{
			//moveDown player should failure, because below we have a water obstacle
			const FPoint startPos = player->GetPos();

			_events->EmitEvent("P1_Move_Down_Pressed");
			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

			EXPECT_EQ(startPos, player->GetPos());

			return;
		}
	}

	EXPECT_TRUE(false);
}

// Check that tank can't move through fortressWall
TEST_F(PlayerTest, TankCantPassThroughfortressWall)
{
	if (const auto player = dynamic_cast<const Player*>(_allObjects.front().get()))
	{
		_allObjects.emplace_back(
				std::make_shared<FortressWall>(
						ObjRectangle{.x = 0.f, .y = _tankSize + 1, .w = _gridSize, .h = _gridSize}, _events,
						&_allObjects, _uuid, _gameMode));

		if (dynamic_cast<FortressWall*>(_allObjects.back().get()))
		{
			//moveDown player should failure, because below we have a fortressWall obstacle
			const FPoint startPos = player->GetPos();

			_events->EmitEvent("P1_Move_Down_Pressed");
			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

			EXPECT_EQ(startPos, player->GetPos());

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(PlayerTest, PlayerTeamWon)
{
	if (const auto player = dynamic_cast<Player*>(_allObjects.front().get()))
	{
		// ** Scene=>Spawn player & Enemy, set enemies HP to 0 then remove enemy from container **

		// Spawn Enemy
		unsigned int _gray{0x808080};
		const ObjRectangle rect2{.x = 0, .y = 0, .w = _tankSize, .h = _tankSize};
		BaseObjProperty baseObjProperty2{.rect = rect2,
										 .color = _gray,
										 .health = _tankHealth,
										 .uuid = _uuid,
										 .name = "Enemy2",
										 .fraction = "EnemyTeam"};
		PawnProperty pawnProperty2{
				.baseObjProperty = std::move(baseObjProperty2),
				.allObjects = &_allObjects,
				.events = _events,
				.tier = 1,
				.speed = _tankSpeed,
				.windowSize = _windowSize,
				.dir = Direction::DOWN,
				.gameMode = _gameMode};
		constexpr bool enableByDefault{true};

		_events->AddListener("PlayersTeamIsWon", _name, [this]()
		{
			this->_isGameWon = true;
		});

		_events->EmitEvent("Reset");

		for (int i = 0; i < 5; ++i)
		{
			_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::ENEMY1));
			_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::ENEMY2));
			_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::ENEMY3));
			_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::ENEMY4));

			_tankSpawner->RespawnTanks(enableByDefault);
			_tankSpawner->RespawnTanks(enableByDefault);
			_tankSpawner->RespawnTanks(enableByDefault);
			_tankSpawner->RespawnTanks(enableByDefault);

			//std::cout << "Entities pretick = " << _allObjects.size() << "\n";  Debug
			for (auto& obj: _allObjects)
			{
				if (auto enemy = dynamic_cast<Enemy*>(obj.get()))
				{
					enemy->SetHealth(0);
				}
			}

			//delete dead tanks
			std::erase_if(_allObjects, [](const auto& obj)
			{
				return obj.get() == nullptr || obj->GetIsAlive() == false;
			});

			//std::cout << "Entities posttick= " << _allObjects.size() << "\n"; Debug
		}

		//Check result
		/*_isGameWon ?
		std::cout << "\nTest result = PASS" << "\n": 
		std::cout << "\nTest result = FAIL" << "\n"; Debug*/

		EXPECT_TRUE(_isGameWon);

		return;
	}
	EXPECT_TRUE(false);

	_events->RemoveListener("PlayersTeamIsWon", _name);
}
