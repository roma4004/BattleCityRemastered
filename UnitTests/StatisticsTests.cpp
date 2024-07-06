#include "MockPlayerOne.h"
#include "../headers/Brick.h"
#include "../headers/Enemy.h"
#include "../headers/EventSystem.h"
#include "../headers/GameStatistics.h"
#include "../headers/InputProviderForPlayerOne.h"
#include "../headers/InputProviderForPlayerTwo.h"
#include "../headers/PlayerTwo.h"

#include "gtest/gtest.h"

#include <memory>

class StatisticsTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events;
	std::shared_ptr<GameStatistics> _statistics;
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
		_statistics = std::make_shared<GameStatistics>(_events);
		const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
		_tankSize = gridSize * 3;// for better turns
		const Rectangle playerRect{0, 0, _tankSize, _tankSize};
		constexpr int yellow = 0xeaea00;
		std::string name = "PlayerOne";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		_bulletPool = std::make_shared<BulletPool>();
		_allObjects.reserve(4);
		_allObjects.emplace_back(std::make_shared<PlayerOne>(playerRect, yellow, _tankHealth, _windowBuffer,
		                                                     _windowSize, UP, _tankSpeed, &_allObjects, _events,
		                                                     name, fraction, inputProvider, _bulletPool));

		const Rectangle player2Rect{_tankSize, 0, _tankSize, _tankSize};
		constexpr int green = 0x408000;
		std::string name2 = "PlayerTwo";
		std::string fraction2 = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider2 = std::make_unique<InputProviderForPlayerTwo>(name2, _events);
		_allObjects.emplace_back(std::make_shared<PlayerTwo>(player2Rect, green, _tankHealth, _windowBuffer,
		                                                     _windowSize, UP, _tankSpeed, &_allObjects, _events,
		                                                     name2, fraction2, inputProvider2, _bulletPool));
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

TEST_F(StatisticsTest, PlayerOneHitByEnemy)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		player->SetPos({0.f, 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Enemy1", "EnemyTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			EXPECT_EQ(_statistics->GetPlayerOneHitByEnemyTeam(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetPlayerOneHitByEnemyTeam(), 1);

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(StatisticsTest, PlayerOneHitByFriend)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		player->SetPos({0.f, 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "PlayerTwo", "PlayerTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			EXPECT_EQ(_statistics->GetPlayerOneHitFriendlyFire(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetPlayerOneHitFriendlyFire(), 1);

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(StatisticsTest, PlayerTwoHitByEnemy)
{
	if (const auto player = dynamic_cast<PlayerTwo*>(_allObjects.back().get()))
	{
		player->SetPos({0.f, 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Enemy1", "EnemyTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			EXPECT_EQ(_statistics->GetPlayerTwoHitByEnemyTeam(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetPlayerTwoHitByEnemyTeam(), 1);

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(StatisticsTest, PlayerTwoHitByFriend)
{
	if (const auto player = dynamic_cast<PlayerTwo*>(_allObjects.back().get()))
	{
		player->SetPos({0.f, 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "PlayerOne", "PlayerTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			EXPECT_EQ(_statistics->GetPlayerTwoHitFriendlyFire(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetPlayerTwoHitFriendlyFire(), 1);

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(StatisticsTest, EnemyOneRespawnNeededFlag)
{
	EXPECT_EQ(_statistics->IsEnemyOneNeedRespawn(), false);
	{
		const Rectangle rect{_tankSize * 2, 0, _tankSize, _tankSize};
		constexpr int gray = 0x808080;
		auto enemy = std::make_unique<Enemy>(rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN, _tankSpeed,
		                                     &_allObjects, _events, "Enemy1", "EnemyTeam", _bulletPool);
	}
	EXPECT_EQ(_statistics->IsEnemyOneNeedRespawn(), true);
}

TEST_F(StatisticsTest, EnemyTwoRespawnNeededFlag)
{
	EXPECT_EQ(_statistics->IsEnemyTwoNeedRespawn(), false);
	{
		const Rectangle rect{_tankSize * 2, 0, _tankSize, _tankSize};
		constexpr int gray = 0x808080;
		auto enemy = std::make_unique<Enemy>(rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN, _tankSpeed,
		                                     &_allObjects, _events, "Enemy2", "EnemyTeam", _bulletPool);
	}
	EXPECT_EQ(_statistics->IsEnemyTwoNeedRespawn(), true);
}

TEST_F(StatisticsTest, EnemyThreeRespawnNeededFlag)
{
	EXPECT_EQ(_statistics->IsEnemyThreeNeedRespawn(), false);
	{
		const Rectangle rect{_tankSize * 2, 0, _tankSize, _tankSize};
		constexpr int gray = 0x808080;
		auto enemy = std::make_unique<Enemy>(rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN, _tankSpeed,
		                                     &_allObjects, _events, "Enemy3", "EnemyTeam", _bulletPool);
	}
	EXPECT_EQ(_statistics->IsEnemyThreeNeedRespawn(), true);
}

TEST_F(StatisticsTest, EnemyFourRespawnNeededFlag)
{
	EXPECT_EQ(_statistics->IsEnemyFourNeedRespawn(), false);
	{
		const Rectangle rect{_tankSize * 2, 0, _tankSize, _tankSize};
		constexpr int gray = 0x808080;
		auto enemy = std::make_unique<Enemy>(rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN, _tankSpeed,
		                                     &_allObjects, _events, "Enemy4", "EnemyTeam", _bulletPool);
	}
	EXPECT_EQ(_statistics->IsEnemyFourNeedRespawn(), true);
}

TEST_F(StatisticsTest, PlayerOneDiedRespawnNeededFlag)
{
	EXPECT_EQ(_statistics->IsPlayerOneNeedRespawn(), false);
	{
		const Rectangle rect{_tankSize * 2, 0, _tankSize, _tankSize};
		constexpr int yellow = 0xeaea00;
		std::string name = "PlayerOne";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		auto enemy = std::make_shared<PlayerOne>(rect, yellow, _tankHealth, _windowBuffer, _windowSize, UP, _tankSpeed,
		                                         &_allObjects, _events, name, fraction, inputProvider, _bulletPool);
	}
	EXPECT_EQ(_statistics->IsPlayerOneNeedRespawn(), true);
}

TEST_F(StatisticsTest, PlayerTwoDiedRespawnNeededFlag)
{
	EXPECT_EQ(_statistics->IsPlayerTwoNeedRespawn(), false);
	{
		const Rectangle rect{_tankSize * 2, 0, _tankSize, _tankSize};
		constexpr int green = 0x408000;
		std::string name = "PlayerTwo";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		auto enemy = std::make_shared<PlayerTwo>(rect, green, _tankHealth, _windowBuffer, _windowSize, UP, _tankSpeed,
		                                         &_allObjects, _events, name, fraction, inputProvider, _bulletPool);
	}
	EXPECT_EQ(_statistics->IsPlayerTwoNeedRespawn(), true);
}

TEST_F(StatisticsTest, EnemyDiedRespawnCount)
{
	const int respawnResource = _statistics->GetEnemyRespawnResource();
	{
		const Rectangle rect{_tankSize * 2, 0, _tankSize, _tankSize};
		constexpr int gray = 0x808080;
		auto enemy = std::make_unique<Enemy>(rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN, _tankSpeed,
		                                     &_allObjects, _events, "Enemy1", "EnemyTeam", _bulletPool);
	}
	EXPECT_GT(respawnResource, _statistics->GetEnemyRespawnResource());
}

TEST_F(StatisticsTest, PlayerOneDiedRespawnCount)
{
	const int respawnResource = _statistics->GetPlayerOneRespawnResource();
	{
		const Rectangle rect{_tankSize * 2, 0, _tankSize, _tankSize};
		constexpr int yellow = 0xeaea00;
		std::string name = "PlayerOne";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		auto enemy = std::make_shared<PlayerOne>(rect, yellow, _tankHealth, _windowBuffer, _windowSize, UP, _tankSpeed,
		                                         &_allObjects, _events, name, fraction, inputProvider, _bulletPool);
	}
	EXPECT_GT(respawnResource, _statistics->GetPlayerOneRespawnResource());
}

TEST_F(StatisticsTest, PlayerTwoDiedRespawnCount)
{
	const int respawnResource = _statistics->GetPlayerTwoRespawnResource();
	{
		const Rectangle rect{_tankSize * 2, 0, _tankSize, _tankSize};
		constexpr int green = 0x408000;
		std::string name = "PlayerTwo";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		auto enemy = std::make_shared<PlayerTwo>(rect, green, _tankHealth, _windowBuffer, _windowSize, UP, _tankSpeed,
		                                         &_allObjects, _events, name, fraction, inputProvider, _bulletPool);
	}
	EXPECT_GT(respawnResource, _statistics->GetPlayerTwoRespawnResource());
}

TEST_F(StatisticsTest, PlayerOneDiedByFriend)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		player->SetPos({0.f, 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{0, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "PlayerTwo", "PlayerTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			player->SetHealth(1);
			EXPECT_EQ(_statistics->GetPlayerOneDiedByFriendlyFire(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetPlayerOneDiedByFriendlyFire(), 1);

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(StatisticsTest, PlayerTwoDiedByEnemy)
{
	if (const auto player = dynamic_cast<PlayerTwo*>(_allObjects.back().get()))
	{
		player->SetPos({0.f, 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Enemy1", "EnemyTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			player->SetHealth(1);
			EXPECT_EQ(_statistics->GetPlayerDiedByEnemyTeam(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetPlayerDiedByEnemyTeam(), 1);

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(StatisticsTest, PlayerOneDiedByEnemy)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		player->SetPos({0.f, 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Enemy1", "EnemyTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			player->SetHealth(1);
			EXPECT_EQ(_statistics->GetPlayerDiedByEnemyTeam(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetPlayerDiedByEnemyTeam(), 1);

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(StatisticsTest, PlayerTwoDiedByFriend)
{
	if (const auto player = dynamic_cast<PlayerTwo*>(_allObjects.back().get()))
	{
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize + _tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "PlayerOne", "PlayerTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			player->SetHealth(1);
			EXPECT_EQ(_statistics->GetPlayerTwoDiedByFriendlyFire(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetPlayerTwoDiedByFriendlyFire(), 1);

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(StatisticsTest, EnemyHitByFriend)
{
	_allObjects.clear();
	const Rectangle enemy1Rect{_tankSize * 2, 0, _tankSize, _tankSize};
	constexpr int gray = 0x808080;
	_allObjects.emplace_back(std::make_shared<Enemy>(enemy1Rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN,
	                                                 _tankSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam",
	                                                 _bulletPool));
	if (const auto enemy = dynamic_cast<Enemy*>(_allObjects.front().get()))
	{
		enemy->SetPos({0.f, 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Enemy2", "EnemyTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			EXPECT_EQ(_statistics->GetEnemyHitByFriendlyFire(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetEnemyHitByFriendlyFire(), 1);

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(StatisticsTest, EnemyHitByPlayerOne)
{
	_allObjects.clear();
	const Rectangle enemy1Rect{_tankSize * 2, 0, _tankSize, _tankSize};
	constexpr int gray = 0x808080;
	_allObjects.emplace_back(std::make_shared<Enemy>(enemy1Rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN,
	                                                 _tankSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam",
	                                                 _bulletPool));
	if (const auto enemy = dynamic_cast<Enemy*>(_allObjects.front().get()))
	{
		enemy->SetPos({0.f, 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "PlayerOne", "PlayerTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			EXPECT_EQ(_statistics->GetEnemyHitByPlayerOne(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetEnemyHitByPlayerOne(), 1);

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(StatisticsTest, EnemyHitByPlayerTwo)
{
	_allObjects.clear();
	const Rectangle enemy1Rect{_tankSize * 2, 0, _tankSize, _tankSize};
	constexpr int gray = 0x808080;
	_allObjects.emplace_back(std::make_shared<Enemy>(enemy1Rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN,
	                                                 _tankSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam",
	                                                 _bulletPool));
	if (const auto enemy = dynamic_cast<Enemy*>(_allObjects.front().get()))
	{
		enemy->SetPos({0.f, 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "PlayerTwo", "PlayerTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			EXPECT_EQ(_statistics->GetEnemyHitByPlayerTwo(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetEnemyHitByPlayerTwo(), 1);

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(StatisticsTest, EnemyDiedByFriend)
{
	_allObjects.clear();
	const Rectangle enemy1Rect{_tankSize * 2, 0, _tankSize, _tankSize};
	constexpr int gray = 0x808080;
	_allObjects.emplace_back(std::make_shared<Enemy>(enemy1Rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN,
	                                                 _tankSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam",
	                                                 _bulletPool));
	if (const auto enemy = dynamic_cast<Enemy*>(_allObjects.front().get()))
	{
		enemy->SetPos({0.f, 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Enemy2", "EnemyTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			enemy->SetHealth(1);
			EXPECT_EQ(_statistics->GetEnemyDiedByFriendlyFire(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetEnemyDiedByFriendlyFire(), 1);

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(StatisticsTest, EnemyDiedByPlayerOne)
{
	_allObjects.clear();
	const Rectangle enemy1Rect{0, 0, _tankSize, _tankSize};
	constexpr int gray = 0x808080;
	_allObjects.emplace_back(std::make_shared<Enemy>(enemy1Rect, gray, _tankHealth, _windowBuffer, _windowSize,
	                                                 DOWN, _tankSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam",
	                                                 _bulletPool));
	if (const auto enemy = dynamic_cast<Enemy*>(_allObjects.front().get()))
	{
		enemy->SetPos({0.f, 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{0, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "PlayerOne", "PlayerTeam"));

		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			enemy->SetHealth(1);
			EXPECT_EQ(_statistics->GetEnemyDiedByPlayerOne(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetEnemyDiedByPlayerOne(), 1);

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(StatisticsTest, EnemyDiedByPlayerTwo)
{
	_allObjects.clear();
	const Rectangle enemy1Rect{_tankSize * 2, 0, _tankSize, _tankSize};
	constexpr int gray = 0x808080;
	_allObjects.emplace_back(std::make_shared<Enemy>(enemy1Rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN,
	                                                 _tankSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam",
	                                                 _bulletPool));
	if (const auto enemy = dynamic_cast<Enemy*>(_allObjects.front().get()))
	{
		enemy->SetPos({0.f, 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "PlayerTwo", "PlayerTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			enemy->SetHealth(1);
			EXPECT_EQ(_statistics->GetEnemyDiedByPlayerTwo(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetEnemyDiedByPlayerTwo(), 1);

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(StatisticsTest, BulletHitByEnemy)
{
	_allObjects.clear();
	constexpr float bulletWidth = 6;
	constexpr float bulletHeight = 5;
	Rectangle bulletRect{0, _tankSize, bulletWidth, bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, DOWN, _bulletSpeed, &_allObjects,
	                                                  _events, "PlayerOne", "PlayerTeam"));
	if (const auto bullet1 = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Enemy1", "EnemyTeam"));
		if (const auto bullet2 = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			bullet2->SetPos({0, _tankSize + bulletHeight + 1});
			EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0);
			EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1);
			EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 1);

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(StatisticsTest, BulletHitByPlayerOne)
{
	_allObjects.clear();
	constexpr float bulletWidth = 6;
	constexpr float bulletHeight = 5;
	Rectangle bulletRect{0, _tankSize, bulletWidth, bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, DOWN, _bulletSpeed, &_allObjects,
	                                                  _events, "PlayerTwo", "PlayerTeam"));
	if (const auto bullet1 = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "PlayerOne", "PlayerTeam"));
		if (const auto bullet2 = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			bullet2->SetPos({0, _tankSize + bulletHeight + 1});
			EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1);

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(StatisticsTest, BulletHitByPlayerTwo)
{
	_allObjects.clear();
	constexpr float bulletWidth = 6;
	constexpr float bulletHeight = 5;
	Rectangle bulletRect{0, _tankSize, bulletWidth, bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, DOWN, _bulletSpeed, &_allObjects,
	                                                  _events, "PlayerOne", "PlayerTeam"));
	if (const auto bullet1 = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "PlayerTwo", "PlayerTeam"));
		if (const auto bullet2 = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			bullet2->SetPos({0, _tankSize + bulletHeight + 1});
			EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0);
			EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1);
			EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 1);

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(StatisticsTest, BrickDiedByEnemy)
{
	_allObjects.clear();
	constexpr Direction direction = DOWN;
	constexpr float bulletWidth = 6;
	constexpr float bulletHeight = 5;
	const Rectangle bulletRect{0, _tankSize + 1, bulletWidth, bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, direction, _bulletSpeed, &_allObjects,
	                                                  _events, "Enemy1", "EnemyTeam"));
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		constexpr float gridSize = 1;
		const Rectangle rect{0, _tankSize + bulletHeight + 2 + gridSize, gridSize, gridSize};
		_allObjects.emplace_back(std::make_shared<Brick>(rect, _windowBuffer, _windowSize, _events));
		if (const auto brick = dynamic_cast<Brick*>(_allObjects.back().get()))
		{
			EXPECT_EQ(_statistics->GetBrickDiedByEnemyTeam(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetBrickDiedByEnemyTeam(), 1);

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(StatisticsTest, BrickDiedByPlayerOne)
{
	_allObjects.clear();
	constexpr Direction direction = DOWN;
	constexpr float bulletWidth = 6;
	constexpr float bulletHeight = 5;
	const Rectangle bulletRect{0, _tankSize + 1, bulletWidth, bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, direction, _bulletSpeed, &_allObjects,
	                                                  _events, "PlayerOne", "PlayerTeam"));
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		constexpr float gridSize = 1;
		const Rectangle rect{0, _tankSize + bulletHeight + 2 + gridSize, gridSize, gridSize};
		_allObjects.emplace_back(std::make_shared<Brick>(rect, _windowBuffer, _windowSize, _events));
		if (const auto brick = dynamic_cast<Brick*>(_allObjects.back().get()))
		{
			EXPECT_EQ(_statistics->GetBrickDiedByPlayerOne(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetBrickDiedByPlayerOne(), 1);

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(StatisticsTest, BrickDiedByPlayerTwo)
{
	_allObjects.clear();
	constexpr Direction direction = DOWN;
	constexpr float bulletWidth = 6;
	constexpr float bulletHeight = 5;
	const Rectangle bulletRect{0, _tankSize, bulletWidth, bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, direction, _bulletSpeed, &_allObjects,
	                                                  _events, "PlayerTwo", "PlayerTeam"));
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		constexpr float gridSize = 1;
		const Rectangle rect{0, _tankSize + bulletHeight + gridSize, gridSize, gridSize};
		_allObjects.emplace_back(std::make_shared<Brick>(rect, _windowBuffer, _windowSize, _events));
		if (const auto brick = dynamic_cast<Brick*>(_allObjects.back().get()))
		{
			EXPECT_EQ(_statistics->GetBrickDiedByPlayerTwo(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetBrickDiedByPlayerTwo(), 1);

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(StatisticsTest, BulletHitBulletByEnemyAndByEnemy)
{
	_allObjects.clear();
	constexpr float bulletWidth = 6;
	constexpr float bulletHeight = 5;
	const Rectangle bulletRect{0, _tankSize, bulletWidth, bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, DOWN, _bulletSpeed, &_allObjects,
	                                                  _events, "Enemy1", "EnemyTeam"));
	if (const auto bullet1 = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Enemy2", "EnemyTeam"));
		if (const auto bullet2 = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			bullet2->SetPos({0, _tankSize + bulletHeight + 1});
			EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 2);

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(StatisticsTest, BulletHitBulletPlayerOneAndByPlayerTwo)
{
	_allObjects.clear();
	constexpr float bulletWidth = 6;
	constexpr float bulletHeight = 5;
	const Rectangle bulletRect{0, _tankSize, bulletWidth, bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, DOWN, _bulletSpeed, &_allObjects,
	                                                  _events, "PlayerOne", "PlayerTeam"));
	if (const auto bullet1 = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "PlayerTwo", "PlayerTeam"));
		if (const auto bullet2 = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			bullet2->SetPos({0, _tankSize + bulletHeight + 1});
			EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0);
			EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1);
			EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 1);

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(StatisticsTest, BulletHitBulletByEnemyAndByPlayerOne)
{
	_allObjects.clear();
	constexpr float bulletWidth = 6;
	constexpr float bulletHeight = 5;
	const Rectangle bulletRect{0, _tankSize, bulletWidth, bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, DOWN, _bulletSpeed,
	                                                  &_allObjects,
	                                                  _events, "PlayerOne", "PlayerTeam"));
	if (const auto bullet1 = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed,
		                                                  &_allObjects, _events, "Enemy1", "EnemyTeam"));
		if (const auto bullet2 = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			bullet2->SetPos({0, _tankSize + bulletHeight + 1});
			EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 0);
			EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 1);
			EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1);

			return;
		}
	}

	EXPECT_TRUE(false);
}

TEST_F(StatisticsTest, BulletHitBulletByEnemyAndByPlayerTwo)
{
	_allObjects.clear();
	constexpr float bulletWidth = 6;
	constexpr float bulletHeight = 5;
	const Rectangle bulletRect{0, _tankSize, bulletWidth, bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, DOWN, _bulletSpeed, &_allObjects,
	                                                  _events, "PlayerTwo", "PlayerTeam"));
	if (const auto bullet1 = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Enemy1", "EnemyTeam"));
		if (const auto bullet2 = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			bullet2->SetPos({0, _tankSize + bulletHeight + 1});
			EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 0);
			EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 0);

			constexpr float deltaTime = 1.f / 60.f;
			_events->EmitEvent<const float>("TickUpdate", deltaTime);

			EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 1);
			EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 1);

			return;
		}
	}

	EXPECT_TRUE(false);
}
