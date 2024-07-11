#include "../headers/EventSystem.h"
#include "../headers/InputProviderForPlayerOne.h"
#include "../headers/TankSpawner.h"
#include "../headers/bonuses/BonusSystem.h"
#include "../headers/bonuses/bonusGrenade.h"
#include "../headers/obstacles/ObstacleAroundFortress.h"
#include "../headers/pawns/Enemy.h"
#include "../headers/pawns/PlayerOne.h"

#include "gtest/gtest.h"

#include <memory>

class BonusTest : public testing::Test
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
	std::unique_ptr<BonusSystem> _bonusSystem;
	std::shared_ptr<TankSpawner> _tankSpawner;

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_tankSpawner = std::make_shared<TankSpawner>(_windowSize, _windowBuffer, &_allObjects, _events);
		const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
		_tankSize = gridSize * 3;// for better turns
		const Rectangle rect{0, 0, _tankSize, _tankSize};
		constexpr int yellow = 0xeaea00;
		std::string name = "PlayerOne";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		//TODO: initialize with btn names
		_bulletPool = std::make_shared<BulletPool>();
		_bonusSystem = std::make_unique<BonusSystem>(_events, &_allObjects, _windowBuffer, _windowSize);
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

// Check that tank can pick up random bonus
TEST_F(BonusTest, BonusPickUp)
{
	if (dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		const size_t size = _allObjects.size();
		_bonusSystem->SpawnRandomBonus({0.f, _tankSize + 1.f, _tankSize, _tankSize});
		// _bonusSystem->SpawnBonus({0.f, _tankSize + 1, _tankSize, _tankSize}, 0xffffff, 2);
		_events->EmitEvent("S_Pressed");
		constexpr float deltaTime = 1.f / 60.f;
		_events->EmitEvent<const float>("TickUpdate", deltaTime);
		if (const auto bonus = _allObjects.back().get())
		{
			EXPECT_EQ(bonus->GetIsAlive(), false);
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
TEST_F(BonusTest, BonusNotPickUp)
{
	if (dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		const size_t size = _allObjects.size();
		_bonusSystem->SpawnRandomBonus({0.f, _tankSize + 1.f, _tankSize, _tankSize});
		// _bonusSystem->SpawnBonus({0.f, _tankSize + 1, _tankSize, _tankSize}, 0xffffff, 2);
		_events->EmitEvent("W_Pressed");
		constexpr float deltaTime = 1.f / 60.f;
		_events->EmitEvent<const float>("TickUpdate", deltaTime);
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
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;

		_bonusSystem->SpawnBonus({0.f, _tankSize + 1.f, _tankSize, _tankSize}, 0xffffff, 0);
		_events->EmitEvent("S_Pressed");

		const Rectangle rect{_tankSize * 2, _tankSize * 2, _tankSize, _tankSize};
		constexpr int gray = 0x808080;
		const auto enemy = std::make_unique<Enemy>(rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN,
		                                           _tankSpeed,
		                                           &_allObjects, _events, "Enemy1", "EnemyTeam", _bulletPool);

		const FPoint enemyPos = enemy->GetPos();

		_events->EmitEvent<const float>("TickUpdate", deltaTime);

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
		constexpr float deltaTime = 1.f / 60.f;

		_bonusSystem->SpawnBonus({0.f, _tankSize + 1.f, _tankSize, _tankSize}, 0xffffff, 0);
		_events->EmitEvent("W_Pressed");

		const Rectangle rect{_tankSize * 2, _tankSize * 2, _tankSize, _tankSize};
		constexpr int gray = 0x808080;
		const auto enemy = std::make_unique<Enemy>(rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN,
		                                           _tankSpeed,
		                                           &_allObjects, _events, "Enemy1", "EnemyTeam", _bulletPool);

		const FPoint enemyPos = enemy->GetPos();

		_events->EmitEvent<const float>("TickUpdate", deltaTime);

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
		constexpr float deltaTime = 1.f / 60.f;

		_bonusSystem->SpawnBonus({0.f, _tankSize + 1.f, _tankSize, _tankSize}, 0xffffff, 1);
		_events->EmitEvent("S_Pressed");
		_events->EmitEvent<const float>("TickUpdate", deltaTime);
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
		const auto [bulletW, bulletH] = FPoint{6.f, 5.f};
		const Rectangle rect{_tankSize + 1.f, 0.f, bulletW, bulletH};
		_allObjects.emplace_back(std::make_shared<Bullet>(rect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, LEFT, _bulletSpeed, &_allObjects,
		                                                  _events, "Enemy1", "EnemyTeam"));

		if (dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

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
		constexpr float deltaTime = 1.f / 60.f;

		_bonusSystem->SpawnBonus({0.f, _tankSize + 1.f, _tankSize, _tankSize}, 0xffffff, 1);
		_events->EmitEvent("W_Pressed");
		_events->EmitEvent<const float>("TickUpdate", deltaTime);
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
		const auto [bulletW, bulletH] = FPoint{6.f, 5.f};
		const Rectangle rect{_tankSize + 1.f, 0.f, bulletW, bulletH};
		_allObjects.emplace_back(std::make_shared<Bullet>(rect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, LEFT, _bulletSpeed, &_allObjects,
		                                                  _events, "Enemy1", "EnemyTeam"));

		if (dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_NE(playerHealth, player->GetHealth());

			return;
		}

		return;
	}

	EXPECT_TRUE(false);
}

TEST_F(BonusTest, GrenadePickUpEnemyHealthZero)
{
	if (dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;

		_bonusSystem->SpawnBonus({0.f, _tankSize + 1.f, _tankSize, _tankSize}, 0xffffff, 2);
		_events->EmitEvent("S_Pressed");

		const Rectangle rect{_tankSize * 2, _tankSize * 2, _tankSize, _tankSize};
		constexpr int gray = 0x808080;
		const auto enemy = std::make_unique<Enemy>(rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN,
		                                           _tankSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam",
		                                           _bulletPool);

		EXPECT_EQ(enemy->GetHealth(), 100);

		_events->EmitEvent<const float>("TickUpdate", deltaTime);

		if (const auto bonus = _allObjects.back().get())
		{
			EXPECT_EQ(bonus->GetIsAlive(), false);
		}
		else
		{
			EXPECT_TRUE(false);
		}

		EXPECT_EQ(enemy->GetHealth(), 0);

		return;
	}

	EXPECT_TRUE(false);
}


TEST_F(BonusTest, GrenadeNotPickUpEnemyHealthFull)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;

		_bonusSystem->SpawnBonus({0.f, _tankSize + 1.f, _tankSize, _tankSize}, 0xffffff, 2);
		_events->EmitEvent("W_Pressed");

		const Rectangle rect{_tankSize * 2, _tankSize * 2, _tankSize, _tankSize};
		constexpr int gray = 0x808080;
		const auto enemy = std::make_unique<Enemy>(rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN,
		                                           _tankSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam",
		                                           _bulletPool);

		EXPECT_EQ(enemy->GetHealth(), 100);

		_events->EmitEvent<const float>("TickUpdate", deltaTime);

		if (const auto bonus = _allObjects.back().get())
		{
			EXPECT_NE(bonus->GetIsAlive(), false);
		}
		else
		{
			EXPECT_TRUE(false);
		}

		EXPECT_EQ(enemy->GetHealth(), 100);

		return;
	}

	EXPECT_TRUE(false);
}

TEST_F(BonusTest, TankPickUpExtraLife)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;

		_bonusSystem->SpawnBonus({0.f, _tankSize + 1.f, _tankSize, _tankSize}, 0xffffff, 3);
		_events->EmitEvent("S_Pressed");

		const int playerSpawnResource = _tankSpawner->GetPlayerOneRespawnResource();

		_events->EmitEvent<const float>("TickUpdate", deltaTime);

		if (const auto bonus = _allObjects.back().get())
		{
			EXPECT_EQ(bonus->GetIsAlive(), false);
		}
		else
		{
			EXPECT_TRUE(false);
		}

		EXPECT_LT(playerSpawnResource, _tankSpawner->GetPlayerOneRespawnResource());

		return;
	}

	EXPECT_TRUE(false);
}


TEST_F(BonusTest, TankNotPickUpTierTheSame)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;

		_bonusSystem->SpawnBonus({0.f, _tankSize + 1.f, _tankSize, _tankSize}, 0xffffff, 3);
		_events->EmitEvent("W_Pressed");

		const int playerSpawnResource = _tankSpawner->GetPlayerOneRespawnResource();

		_events->EmitEvent<const float>("TickUpdate", deltaTime);

		if (const auto bonus = _allObjects.back().get())
		{
			EXPECT_NE(bonus->GetIsAlive(), false);
		}
		else
		{
			EXPECT_TRUE(false);
		}

		EXPECT_EQ(playerSpawnResource, _tankSpawner->GetPlayerOneRespawnResource());

		return;
	}

	EXPECT_TRUE(false);
}

TEST_F(BonusTest, StarPickUpTierIncrease)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;

		_bonusSystem->SpawnBonus({0.f, _tankSize + 1.f, _tankSize, _tankSize}, 0xffffff, 4);
		_events->EmitEvent("S_Pressed");

		EXPECT_EQ(player->GetTankTier(), 0);

		_events->EmitEvent<const float>("TickUpdate", deltaTime);

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
		constexpr float deltaTime = 1.f / 60.f;

		_bonusSystem->SpawnBonus({0.f, _tankSize + 1.f, _tankSize, _tankSize}, 0xffffff, 4);
		_events->EmitEvent("W_Pressed");

		EXPECT_EQ(player->GetTankTier(), 0);

		_events->EmitEvent<const float>("TickUpdate", deltaTime);

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

TEST_F(BonusTest, ShovelPickUpBrickAroundFortressTurnIntoIron)
{
	if (dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;

		_bonusSystem->SpawnBonus({0.f, _tankSize + 1.f, _tankSize, _tankSize}, 0xffffff, 5);
		_events->EmitEvent("S_Pressed");

		const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
		const Rectangle rect{_tankSize + 1.f, 0, gridSize, gridSize};
		if (const auto obstacleAroundFortress =
				std::make_shared<ObstacleAroundFortress>(rect, _windowBuffer, _windowSize, _events, &_allObjects))
		{
			if (obstacleAroundFortress->IsBrick())
			{
				EXPECT_TRUE(true);
			}
			else
			{
				EXPECT_TRUE(false);
			}

			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			if (const auto bonus = _allObjects.back().get())
			{
				EXPECT_EQ(bonus->GetIsAlive(), false);
			}
			else
			{
				EXPECT_TRUE(false);
			}

			if (!obstacleAroundFortress->IsBrick())
			{
				EXPECT_TRUE(true);
			}
			else
			{
				EXPECT_TRUE(false);
			}
		}

		return;
	}

	EXPECT_TRUE(false);
}


TEST_F(BonusTest, ShovelNotPickUpBrickAroundFortressTheSame)
{
	if (dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		constexpr float deltaTime = 1.f / 60.f;

		_bonusSystem->SpawnBonus({0.f, _tankSize + 1.f, _tankSize, _tankSize}, 0xffffff, 5);
		_events->EmitEvent("W_Pressed");

		const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
		const Rectangle rect{_tankSize + 1.f, 0, gridSize, gridSize};
		if (const auto obstacleAroundFortress =
				std::make_shared<ObstacleAroundFortress>(rect, _windowBuffer, _windowSize, _events, &_allObjects))
		{
			if (obstacleAroundFortress->IsBrick())
			{
				EXPECT_TRUE(true);
			}
			else
			{
				EXPECT_TRUE(false);
			}

			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			if (const auto bonus = _allObjects.back().get())
			{
				EXPECT_NE(bonus->GetIsAlive(), false);
			}
			else
			{
				EXPECT_TRUE(false);
			}

			if (obstacleAroundFortress->IsBrick())
			{
				EXPECT_TRUE(true);
			}
			else
			{
				EXPECT_TRUE(false);
			}
		}

		return;
	}

	EXPECT_TRUE(false);
}
