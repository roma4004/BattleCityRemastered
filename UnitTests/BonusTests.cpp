#include "../headers/application/Window.h"
#include "../headers/components/BonusSpawner.h"
#include "../headers/components/BulletPool.h"
#include "../headers/components/EventSystem.h"
#include "../headers/components/TankSpawner.h"
#include "../headers/enums/BonusType.h"
#include "../headers/enums/Direction.h"
#include "../headers/enums/GameMode.h"
#include "../headers/input/InputProviderForPlayerOne.h"
#include "../headers/obstacles/FortressWall.h"
#include "../headers/pawns/Bullet.h"
#include "../headers/pawns/Enemy.h"
#include "../headers/pawns/PlayerOne.h"

#include "gtest/gtest.h"

#include <memory>

class BonusTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::unique_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<Window> _window{nullptr};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;

	int _tankHealth{100};
	int _yellow{0xeaea00};
	int _gray{0x808080};
	GameMode _gameMode{OnePlayer};
	int _bulletColor{0xffffff};
	int _bulletHealth{1};
	int _bulletDamage{1};
	double _bulletDamageRadius{12.0};
	float _tankSize{};
	float _gridSize{};
	float _tankSpeed{142};
	float _bulletSpeed{300.f};
	float _deltaTimeOneFrame{1.f / 60.f};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_window = std::make_shared<Window>(UPoint{.x = 800, .y = 600}, std::shared_ptr<int[]>());
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _window, _gameMode);
		_tankSpawner = std::make_shared<TankSpawner>(_window, &_allObjects, _events, _bulletPool);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, &_allObjects, _window);
		_gridSize = static_cast<float>(_window->size.y) / 50.f;
		_tankSize = _gridSize * 3;// for better turns
		std::string name = "Player";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);

		_allObjects.reserve(4);
		_allObjects.emplace_back(
				std::make_shared<PlayerOne>(
						ObjRectangle{.x = 0, .y = 0, .w = _tankSize, .h = _tankSize}, _yellow, _tankHealth, _window, UP,
						_tankSpeed, &_allObjects, _events, name, fraction, std::move(inputProvider), _bulletPool,
						_gameMode, 1));
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

// Check that tank can pick up random bonus
TEST_F(BonusTest, BonusPickUp)
{
	const size_t size = _allObjects.size();
	_bonusSpawner->SpawnRandomBonus({.x = 0.f, .y = _tankSize + 2.f, .w = _tankSize, .h = _tankSize});
	_events->EmitEvent("S_Pressed");

	if (const auto bonus = _allObjects.back().get())
	{
		EXPECT_TRUE(bonus->GetIsAlive());

		_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

		EXPECT_FALSE(bonus->GetIsAlive());
		EXPECT_LT(size, _allObjects.size());
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

// Check that tank can pick up random bonus
TEST_F(BonusTest, BonusNotPickUp)
{
	if (dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		const size_t size = _allObjects.size();
		_bonusSpawner->SpawnRandomBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize});
		_events->EmitEvent("W_Pressed");
		_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);
		if (const auto bonus = _allObjects.back().get())
		{
			EXPECT_NE(bonus->GetIsAlive(), false);
			EXPECT_LT(size, _allObjects.size());
		}
		else
		{
			EXPECT_TRUE(false);
		}

		return;
	}

	EXPECT_TRUE(false);
}

//TODO: check if enemy pick up timer player team should freeze
// Check that tank can pick up random bonus
TEST_F(BonusTest, TimerPickUpEnemyCantMove)
{
	if (dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		_bonusSpawner->SpawnBonus(
				{.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor, Timer);
		_events->EmitEvent("S_Pressed");

		const auto enemy = std::make_unique<Enemy>(
				ObjRectangle{.x = _tankSize * 2, .y = _tankSize * 2, .w = _tankSize, .h = _tankSize}, _gray,
				_tankHealth, _window, DOWN, _tankSpeed, &_allObjects, _events, "Enemy", "EnemyTeam", _bulletPool,
				_gameMode, 1);

		const FPoint enemyPos = enemy->GetPos();

		_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

		EXPECT_EQ(enemyPos, enemy->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that tank can pick up random bonus
TEST_F(BonusTest, TimerNotPickUpEnemyCanMove)
{
	if (dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		_bonusSpawner->SpawnBonus(
				{.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor, Timer);
		_events->EmitEvent("W_Pressed");

		const auto enemy = std::make_unique<Enemy>(
				ObjRectangle{.x = _tankSize * 2, .y = _tankSize * 2, .w = _tankSize, .h = _tankSize}, _gray,
				_tankHealth, _window, DOWN, _tankSpeed, &_allObjects, _events, "Enemy", "EnemyTeam", _bulletPool,
				_gameMode, 1);

		const FPoint enemyPos = enemy->GetPos();

		_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

		EXPECT_NE(enemyPos, enemy->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

TEST_F(BonusTest, HelmetPickUpBulletCantDamageTank)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		const int playerHealth = player->GetHealth();

		_bonusSpawner->SpawnBonus(
				{.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor, Helmet);
		_events->EmitEvent("S_Pressed");
		_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);
		if (const auto bonus = _allObjects.back().get())
		{
			EXPECT_EQ(bonus->GetIsAlive(), false);
		}
		else
		{
			EXPECT_TRUE(false);
		}

		_allObjects.emplace_back(
				std::make_shared<Bullet>(
						ObjRectangle{.x = _tankSize + 1.f, .y = 0.f, .w = 6.f, .h = 5.f}, _bulletDamage,
						_bulletDamageRadius, _bulletColor, _bulletHealth, _window, LEFT, _bulletSpeed, &_allObjects,
						_events, "Enemy1", "EnemyTeam", _gameMode, 1));

		if (dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

			EXPECT_EQ(playerHealth, player->GetHealth());

			return;
		}

		return;
	}

	EXPECT_TRUE(false);
}

TEST_F(BonusTest, HelmetNotPickUpBulletCanDamageTank)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		const int playerHealth = player->GetHealth();

		_bonusSpawner->SpawnBonus(
				{.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor, Helmet);
		_events->EmitEvent("W_Pressed");
		_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);
		if (const auto bonus = _allObjects.back().get())
		{
			EXPECT_NE(bonus->GetIsAlive(), false);
		}
		else
		{
			EXPECT_TRUE(false);
		}

		_allObjects.emplace_back(
				std::make_shared<Bullet>(
						ObjRectangle{.x = _tankSize + 1.f, .y = 0.f, .w = 6.f, .h = 5.f}, _bulletDamage,
						_bulletDamageRadius, _bulletColor, _bulletHealth, _window, LEFT, _bulletSpeed, &_allObjects,
						_events, "Enemy1", "EnemyTeam", _gameMode, 1));

		_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

		EXPECT_NE(playerHealth, player->GetHealth());

		return;
	}

	EXPECT_TRUE(false);
}

TEST_F(BonusTest, GrenadePickUpEnemyHealthZero)
{
	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor, Grenade);
	_events->EmitEvent("S_Pressed");

	const auto enemy = std::make_unique<Enemy>(
			ObjRectangle{.x = _tankSize * 2, .y = _tankSize * 2, .w = _tankSize, .h = _tankSize}, _gray, _tankHealth,
			_window, DOWN, _tankSpeed, &_allObjects, _events, "Enemy", "EnemyTeam", _bulletPool, _gameMode, 1);

	EXPECT_EQ(enemy->GetHealth(), 100);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	if (const auto bonus = _allObjects.back().get())
	{
		EXPECT_EQ(bonus->GetIsAlive(), false);
	}
	else
	{
		EXPECT_TRUE(false);
	}

	EXPECT_EQ(enemy->GetHealth(), 0);
}

TEST_F(BonusTest, GrenadeNotPickUpEnemyHealthFull)
{
	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor, Grenade);
	_events->EmitEvent("W_Pressed");

	const auto enemy = std::make_unique<Enemy>(
			ObjRectangle{.x = _tankSize * 2, .y = _tankSize * 2, .w = _tankSize, .h = _tankSize}, _gray, _tankHealth,
			_window, DOWN, _tankSpeed, &_allObjects, _events, "Enemy", "EnemyTeam", _bulletPool, _gameMode, 1);

	EXPECT_EQ(enemy->GetHealth(), 100);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	if (const auto bonus = _allObjects.back().get())
	{
		EXPECT_NE(bonus->GetIsAlive(), false);
	}
	else
	{
		EXPECT_TRUE(false);
	}

	EXPECT_EQ(enemy->GetHealth(), 100);
}

TEST_F(BonusTest, TankPickUpExtraLife)
{
	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor, Tank);
	_events->EmitEvent("S_Pressed");

	const int playerSpawnResource = _tankSpawner->GetPlayerOneRespawnResource();
	const auto bonus = _allObjects.back().get();

	EXPECT_EQ(bonus->GetIsAlive(), true);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(bonus->GetIsAlive(), false);

	EXPECT_LT(playerSpawnResource, _tankSpawner->GetPlayerOneRespawnResource());
}

TEST_F(BonusTest, TankNotPickUpTierTheSame)
{
	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor, Tank);
	_events->EmitEvent("W_Pressed");

	const int playerSpawnResource = _tankSpawner->GetPlayerOneRespawnResource();

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	if (const auto bonus = _allObjects.back().get())
	{
		EXPECT_NE(bonus->GetIsAlive(), false);
	}
	else
	{
		EXPECT_TRUE(false);
	}

	EXPECT_EQ(playerSpawnResource, _tankSpawner->GetPlayerOneRespawnResource());
}

TEST_F(BonusTest, StarPickUpTierIncrease)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor, Star);
		_events->EmitEvent("S_Pressed");

		EXPECT_EQ(player->GetTier(), 1);

		_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

		if (const auto bonus = _allObjects.back().get())
		{
			EXPECT_EQ(bonus->GetIsAlive(), false);
		}
		else
		{
			EXPECT_TRUE(false);
		}

		EXPECT_EQ(player->GetTier(), 2);

		return;
	}

	EXPECT_TRUE(false);
}

TEST_F(BonusTest, StarNotPickUpTierTheSame)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor, Star);
		_events->EmitEvent("W_Pressed");

		EXPECT_EQ(player->GetTier(), 1);

		_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

		if (const auto bonus = _allObjects.back().get())
		{
			EXPECT_NE(bonus->GetIsAlive(), false);
		}
		else
		{
			EXPECT_TRUE(false);
		}

		EXPECT_EQ(player->GetTier(), 1);

		return;
	}

	EXPECT_TRUE(false);
}

// NOTE: when player pick up shovel bonus fortressWalls become steelWalls for a while then return to regular brickWalls
TEST_F(BonusTest, ShovelPickUpByPlayerThenFortressWallTurnIntoSteelWall)
{
	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor, Shovel);
	_events->EmitEvent("S_Pressed");

	const auto fortressWall =
			std::make_shared<FortressWall>(ObjRectangle{.x = _tankSize + 1.f, .y = 0, .w = _gridSize, .h = _gridSize},
			                               _window, _events, &_allObjects, 0, _gameMode);

	EXPECT_TRUE(fortressWall->IsBrickWall());

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_TRUE(fortressWall->IsSteelWall());
}

//TODO: check that player can pickup bonus and rebuild fortress and skip if space spawn not available
TEST_F(BonusTest, ShovelNotPickUpByPlayerThenfortressWallRemainTheSame)
{
	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor, Shovel);
	_events->EmitEvent("W_Pressed");

	const auto fortressWall =
			std::make_shared<FortressWall>(ObjRectangle{.x = _tankSize + 1.f, .y = 0, .w = _gridSize, .h = _gridSize},
			                               _window, _events, &_allObjects, 0, _gameMode);

	EXPECT_TRUE(fortressWall->IsBrickWall());

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_TRUE(fortressWall->IsBrickWall());
}

// NOTE: when enemy pick up shovel bonus fortressWalls hide (destroyed behaviour)
TEST_F(BonusTest, ShovelPickUpByEnemyThenFortressWallBrickHide)
{
	_allObjects.clear();
	auto bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _window, _gameMode);
	_allObjects.emplace_back(
			std::make_shared<Enemy>(
					ObjRectangle{.x = 0, .y = 0, .w = _tankSize, .h = _tankSize}, _gray, _tankHealth, _window, DOWN,
					_tankSpeed, &_allObjects, _events, "Enemy", "EnemyTeam", bulletPool, _gameMode, 1));

	_allObjects.emplace_back(
			std::make_shared<FortressWall>(
					ObjRectangle{.x = _tankSize + 1.f, .y = 0, .w = _gridSize, .h = _gridSize}, _window, _events,
					&_allObjects, 0, _gameMode));
	const auto fortressWall = dynamic_cast<FortressWall*>(_allObjects.back().get());

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor, Shovel);

	EXPECT_TRUE(fortressWall->IsBrickWall());
	EXPECT_NE(fortressWall->GetHealth(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_TRUE(fortressWall->IsBrickWall());
	EXPECT_EQ(fortressWall->GetHealth(), 0);
}

// NOTE: when player pick up shovel bonus fortressWalls become steelWalls (BonusShovelSwitch)
//       then enemy pick up shovel bonus fortressWalls should hide (destroy)
TEST_F(BonusTest, ShovelPickUpByEnemyThenFortressWallSteelWallHide)
{
	_allObjects.clear();
	auto bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _window, _gameMode);
	_allObjects.emplace_back(
			std::make_shared<Enemy>(
					ObjRectangle{.x = 0, .y = 0, .w = _tankSize, .h = _tankSize}, _gray, _tankHealth, _window, DOWN,
					_tankSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam", bulletPool, _gameMode, 1));

	_allObjects.emplace_back(
			std::make_shared<FortressWall>(
					ObjRectangle{.x = _tankSize + 1.f, .y = 0, .w = _tankSize, .h = _tankSize}, _window, _events,
					&_allObjects, 0, _gameMode));
	if (const auto fortressWall = dynamic_cast<FortressWall*>(_allObjects.back().get()))
	{
		EXPECT_TRUE(fortressWall->IsBrickWall());
		fortressWall->OnPlayerPickupShovel();
		//TODO: rewrite scenario P1 firstly pick up bonus shovel, then create enemy that pick up bonus shovel
		EXPECT_TRUE(fortressWall->IsSteelWall());

		_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor, Shovel);

		EXPECT_TRUE(fortressWall->IsSteelWall());
		EXPECT_NE(fortressWall->GetHealth(), 0);

		_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

		EXPECT_TRUE(fortressWall->IsBrickWall());
		EXPECT_EQ(fortressWall->GetHealth(), 0);

		return;
	}

	EXPECT_TRUE(false);
}
