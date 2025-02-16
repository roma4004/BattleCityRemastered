#include "../headers/EventSystem.h"
#include "../headers/GameMode.h"
#include "../headers/TankSpawner.h"
#include "../headers/bonuses/BonusShovel.h"
#include "../headers/bonuses/BonusSpawner.h"
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
	float _tankSize{};
	float _gridSize{};
	float _tankSpeed{142};
	float _bulletSpeed{300.f};
	float _deltaTimeOneFrame{1.f / 60.f};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		GameMode currentMode = OnePlayer;
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _windowSize, _windowBuffer, currentMode);
		_tankSpawner = std::make_shared<TankSpawner>(_windowSize, _windowBuffer, &_allObjects, _events, _bulletPool);
		_gridSize = static_cast<float>(_windowSize.y) / 50.f;
		_tankSize = _gridSize * 3;// for better turns
		const ObjRectangle rect{.x = 0, .y = 0, .w = _tankSize, .h = _tankSize};
		constexpr int yellow = 0xeaea00;
		std::string name = "Player";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		//TODO: initialize with btn names
		auto currentGameMode = OnePlayer;
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _windowSize, _windowBuffer, currentGameMode);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, &_allObjects, _windowBuffer, _windowSize);
		_allObjects.reserve(4);
		_allObjects.emplace_back(
				std::make_shared<PlayerOne>(
						rect, yellow, _tankHealth, _windowBuffer, _windowSize, UP, _tankSpeed, &_allObjects, _events,
						name, fraction, std::move(inputProvider), _bulletPool, false, 1));
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
	_bonusSpawner->SpawnRandomBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize});
	// _bonusSpawner->SpawnBonus({0.f, _tankSize + 1, _tankSize, _tankSize}, 0xffffff, 2);
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
		// _bonusSpawner->SpawnBonus({0.f, _tankSize + 1, _tankSize, _tankSize}, 0xffffff, 2);
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
		_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, 0xffffff, 0);
		_events->EmitEvent("S_Pressed");

		const ObjRectangle rect{.x = _tankSize * 2, .y = _tankSize * 2, .w = _tankSize, .h = _tankSize};
		constexpr int gray = 0x808080;
		const auto enemy = std::make_unique<Enemy>(
				rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN, _tankSpeed, &_allObjects, _events, "Enemy",
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
		_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, 0xffffff, 0);
		_events->EmitEvent("W_Pressed");

		const ObjRectangle rect{.x = _tankSize * 2, .y = _tankSize * 2, .w = _tankSize, .h = _tankSize};
		constexpr int gray = 0x808080;
		const auto enemy = std::make_unique<Enemy>(
				rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN, _tankSpeed, &_allObjects, _events, "Enemy",
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

		_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, 0xffffff, 1);
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

		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		const auto [bulletW, bulletH] = FPoint{.x = 6.f, .y = 5.f};
		const ObjRectangle rect{.x = _tankSize + 1.f, .y = 0.f, .w = bulletW, .h = bulletH};
		_allObjects.emplace_back(
				std::make_shared<Bullet>(
						rect, damage, bulletDamageAreaRadius, color, health, _windowBuffer, _windowSize, LEFT,
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

		_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, 0xffffff, 1);
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

		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		const auto [bulletW, bulletH] = FPoint{.x = 6.f, .y = 5.f};
		const ObjRectangle rect{.x = _tankSize + 1.f, .y = 0.f, .w = bulletW, .h = bulletH};
		_allObjects.emplace_back(
				std::make_shared<Bullet>(
						rect, damage, bulletDamageAreaRadius, color, health, _windowBuffer, _windowSize, LEFT,
						_bulletSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam", 0, false));

		_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

		EXPECT_NE(playerHealth, player->GetHealth());

		return;
	}

	EXPECT_TRUE(false);
}

TEST_F(BonusTest, GrenadePickUpEnemyHealthZero)
{
	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, 0xffffff, 2);
	_events->EmitEvent("S_Pressed");

	const ObjRectangle rect{.x = _tankSize * 2, .y = _tankSize * 2, .w = _tankSize, .h = _tankSize};
	constexpr int gray = 0x808080;
	const auto enemy = std::make_unique<Enemy>(rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN, _tankSpeed,
	                                           &_allObjects, _events, "Enemy", "EnemyTeam", _bulletPool, false, 1);

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
	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, 0xffffff, 2);
	_events->EmitEvent("W_Pressed");

	const ObjRectangle rect{.x = _tankSize * 2, .y = _tankSize * 2, .w = _tankSize, .h = _tankSize};
	constexpr int gray = 0x808080;
	const auto enemy = std::make_unique<Enemy>(rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN, _tankSpeed,
	                                           &_allObjects, _events, "Enemy", "EnemyTeam", _bulletPool, false, 1);

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
	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, 0xffffff, 3);
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
	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, 0xffffff, 3);
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
		_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, 0xffffff, 4);
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
		_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, 0xffffff, 4);
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
	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, 0xffffff, 5);
	_events->EmitEvent("S_Pressed");

	const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
	const ObjRectangle rect{.x = _tankSize + 1.f, .y = 0, .w = gridSize, .h = gridSize};
	const auto fortressWall =
			std::make_shared<FortressWall>(rect, _windowBuffer, _windowSize, _events, &_allObjects, 0);

	EXPECT_TRUE(fortressWall->IsBrickWall());

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_TRUE(fortressWall->IsSteelWall());
}

TEST_F(BonusTest, ShovelNotPickUpByPlayerThenfortressWallRemainTheSame)
{
	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, 0xffffff, 5);
	_events->EmitEvent("W_Pressed");

	const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
	const ObjRectangle rect{.x = _tankSize + 1.f, .y = 0, .w = gridSize, .h = gridSize};
	const auto fortressWall =
			std::make_shared<FortressWall>(rect, _windowBuffer, _windowSize, _events, &_allObjects, 0);

	EXPECT_TRUE(fortressWall->IsBrickWall());

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_TRUE(fortressWall->IsBrickWall());
}

//TODO: FIX THIS
TEST_F(BonusTest, ShovelPickUpByEnemyThenfortressWallBrickHide)
{
	_allObjects.clear();
	const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
	const float tankSize = gridSize * 3;// for better turns
	constexpr float tankSpeed{142};
	constexpr int tankHealth = 100;
	constexpr int gray = 0x808080;
	auto currentGameMode = OnePlayer;
	auto bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _windowSize, _windowBuffer, currentGameMode);
	_allObjects.emplace_back(
			std::make_shared<Enemy>(
					ObjRectangle{.x = 0, .y = 0, .w = tankSize, .h = tankSize}, gray, tankHealth, _windowBuffer,
					_windowSize, DOWN, tankSpeed, &_allObjects, _events, "Enemy", "EnemyTeam", bulletPool, false, 1));

	_allObjects.emplace_back(
			std::make_shared<FortressWall>(
					ObjRectangle{.x = _tankSize + 1.f, .y = 0, .w = gridSize, .h = gridSize}, _windowBuffer,
					_windowSize, _events, &_allObjects, 0));
	const auto fortressWall = dynamic_cast<FortressWall*>(_allObjects.back().get());

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, 0xffffff, 5);

	EXPECT_TRUE(fortressWall->IsBrickWall());
	EXPECT_TRUE(fortressWall->GetIsAlive());

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_TRUE(fortressWall->IsBrickWall());
	EXPECT_FALSE(fortressWall->GetIsAlive());
}

// player pick up shovel bonus so fortressWalls is steelWalls then enemy pick up shovel bonus walls should hide (destroy)
TEST_F(BonusTest, ShovelPickUpByEnemyThenfortressWallSteelWallHide)
{
	_allObjects.clear();
	_allObjects.emplace_back(
			std::make_shared<FortressWall>(
					ObjRectangle{.x = _tankSize + 1.f, .y = 0, .w = _tankSize, .h = _tankSize}, _windowBuffer,
					_windowSize, _events, &_allObjects, 0));
	if (const auto fortressWall = dynamic_cast<FortressWall*>(_allObjects.back().get()))
	{
		EXPECT_TRUE(fortressWall->IsBrickWall());
		fortressWall->BonusShovelSwitch();
		EXPECT_TRUE(fortressWall->IsSteelWall());

		constexpr int gray = 0x808080;
		constexpr int tankHealth = 100;
		constexpr float tankSpeed{142};
		auto currentGameMode = OnePlayer;
		auto bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _windowSize, _windowBuffer,
		                                               currentGameMode);
		_allObjects.emplace_back(
				std::make_shared<Enemy>(
						ObjRectangle{.x = 0, .y = 0, .w = _tankSize, .h = _tankSize}, gray, tankHealth, _windowBuffer,
						_windowSize, DOWN, tankSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam", bulletPool, false,
						1));
		_bonusSpawner->SpawnBonus(
				{.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, 0xffffff, 5);

		EXPECT_TRUE(fortressWall->IsSteelWall());
		EXPECT_TRUE(fortressWall->GetIsAlive());

		_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

		EXPECT_TRUE(fortressWall->IsSteelWall());
		EXPECT_FALSE(fortressWall->GetIsAlive());

		return;
	}

	EXPECT_TRUE(false);
}
