#include "../headers/EventSystem.h"
#include "../headers/GameMode.h"
#include "../headers/TankSpawner.h"
#include "../headers/bonuses/BonusShovel.h"
#include "../headers/bonuses/BonusSpawner.h"
#include "../headers/bonuses/BonusTypeId.h"
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
	std::shared_ptr<int[]> _windowBuffer{nullptr};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	UPoint _windowSize{.x = 800, .y = 600};
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
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _windowSize, _windowBuffer, _gameMode);
		_tankSpawner = std::make_shared<TankSpawner>(_windowSize, _windowBuffer, &_allObjects, _events, _bulletPool);
		_gridSize = static_cast<float>(_windowSize.y) / 50.f;
		_tankSize = _gridSize * 3;// for better turns

		std::string name = "Player";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		//TODO: initialize with btn names
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _windowSize, _windowBuffer, _gameMode);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, &_allObjects, _windowBuffer, _windowSize);
		_allObjects.reserve(4);
		_allObjects.emplace_back(
				std::make_shared<PlayerOne>(
						ObjRectangle{.x = 0, .y = 0, .w = _tankSize, .h = _tankSize}, _yellow, _tankHealth,
						_windowBuffer, _windowSize, UP, _tankSpeed, &_allObjects, _events, name, fraction,
						std::move(inputProvider), _bulletPool, false, 1));
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

// Check that tank can pick up random bonus
TEST_F(BonusTest, TimerPickUpEnemyCantMove)
{
	if (dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor,
		                          Timer);
		_events->EmitEvent("S_Pressed");

		const auto enemy = std::make_unique<Enemy>(
				ObjRectangle{.x = _tankSize * 2, .y = _tankSize * 2, .w = _tankSize, .h = _tankSize}, _gray,
				_tankHealth, _windowBuffer, _windowSize, DOWN, _tankSpeed, &_allObjects, _events, "Enemy",
				"EnemyTeam", _bulletPool, false, 1);

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
		_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor,
		                          Timer);
		_events->EmitEvent("W_Pressed");

		const auto enemy = std::make_unique<Enemy>(
				ObjRectangle{.x = _tankSize * 2, .y = _tankSize * 2, .w = _tankSize, .h = _tankSize}, _gray,
				_tankHealth, _windowBuffer, _windowSize, DOWN, _tankSpeed, &_allObjects, _events, "Enemy",
				"EnemyTeam", _bulletPool, false, 1);

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

		_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor,
		                          Helmet);
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
						_bulletDamageRadius, _bulletColor, _bulletHealth, _windowBuffer, _windowSize, LEFT,
						_bulletSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam", 0, false));

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

		_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor,
		                          Helmet);
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
						_bulletDamageRadius, _bulletColor, _bulletHealth, _windowBuffer, _windowSize, LEFT,
						_bulletSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam", 0, false));

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
			_windowBuffer, _windowSize, DOWN, _tankSpeed, &_allObjects, _events, "Enemy", "EnemyTeam", _bulletPool,
			false, 1);

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
			_windowBuffer, _windowSize, DOWN, _tankSpeed, &_allObjects, _events, "Enemy", "EnemyTeam", _bulletPool,
			false, 1);

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

		EXPECT_EQ(player->GetTankTier(), 0);

		_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

		if (const auto bonus = _allObjects.back().get())
		{
			EXPECT_EQ(bonus->GetIsAlive(), false);
		}
		else
		{
			EXPECT_TRUE(false);
		}

		EXPECT_EQ(player->GetTankTier(), 1);

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

		EXPECT_EQ(player->GetTankTier(), 0);

		_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

		if (const auto bonus = _allObjects.back().get())
		{
			EXPECT_NE(bonus->GetIsAlive(), false);
		}
		else
		{
			EXPECT_TRUE(false);
		}

		EXPECT_EQ(player->GetTankTier(), 0);

		return;
	}

	EXPECT_TRUE(false);
}

TEST_F(BonusTest, ShovelPickUpByPlayerThenfortressWallTurnIntoSteelWall)
{
	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor, Shovel);
	_events->EmitEvent("S_Pressed");

	const auto fortressWall =
			std::make_shared<FortressWall>(ObjRectangle{.x = _tankSize + 1.f, .y = 0, .w = _gridSize, .h = _gridSize},
			                               _windowBuffer, _windowSize, _events, &_allObjects, 0);

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
			                               _windowBuffer, _windowSize, _events, &_allObjects, 0);

	EXPECT_TRUE(fortressWall->IsBrickWall());

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_TRUE(fortressWall->IsBrickWall());
}

TEST_F(BonusTest, ShovelPickUpByEnemyThenfortressWallBrickHide)
{
	_allObjects.clear();
	auto bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _windowSize, _windowBuffer, _gameMode);
	_allObjects.emplace_back(
			std::make_shared<Enemy>(
					ObjRectangle{.x = 0, .y = 0, .w = _tankSize, .h = _tankSize}, _gray, _tankHealth, _windowBuffer,
					_windowSize, DOWN, _tankSpeed, &_allObjects, _events, "Enemy", "EnemyTeam", bulletPool, false, 1));

	_allObjects.emplace_back(
			std::make_shared<FortressWall>(
					ObjRectangle{.x = _tankSize + 1.f, .y = 0, .w = _gridSize, .h = _gridSize}, _windowBuffer,
					_windowSize, _events, &_allObjects, 0));
	const auto fortressWall = dynamic_cast<FortressWall*>(_allObjects.back().get());

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor, Shovel);

	EXPECT_TRUE(fortressWall->IsBrickWall());
	EXPECT_TRUE(fortressWall->GetIsAlive());

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_TRUE(fortressWall->IsBrickWall());
	EXPECT_FALSE(fortressWall->GetIsAlive());
}

// NOTE: when player pick up shovel bonus fortressWalls become steelWalls (BonusShovelSwitch)
//       then enemy pick up shovel bonus fortressWalls should hide (destroy)
TEST_F(BonusTest, ShovelPickUpByEnemyThenfortressWallSteelWallHide)
{
	_allObjects.clear();
	auto bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _windowSize, _windowBuffer, _gameMode);
	_allObjects.emplace_back(
			std::make_shared<Enemy>(
					ObjRectangle{.x = 0, .y = 0, .w = _tankSize, .h = _tankSize}, _gray, _tankHealth, _windowBuffer,
					_windowSize, DOWN, _tankSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam", bulletPool, false,
					1));

	_allObjects.emplace_back(
			std::make_shared<FortressWall>(
					ObjRectangle{.x = _tankSize + 1.f, .y = 0, .w = _tankSize, .h = _tankSize}, _windowBuffer,
					_windowSize, _events, &_allObjects, 0));
	if (const auto fortressWall = dynamic_cast<FortressWall*>(_allObjects.back().get()))
	{
		EXPECT_TRUE(fortressWall->IsBrickWall());
		fortressWall->BonusShovelSwitch();
		EXPECT_TRUE(fortressWall->IsSteelWall());

		_bonusSpawner->SpawnBonus(
				{.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, 0xffffff, Shovel);

		EXPECT_TRUE(fortressWall->IsSteelWall());
		EXPECT_TRUE(fortressWall->GetIsAlive());

		_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

		EXPECT_TRUE(fortressWall->IsSteelWall());
		EXPECT_FALSE(fortressWall->GetIsAlive());

		return;
	}

	EXPECT_TRUE(false);
}
