#include "../headers/EventSystem.h"
#include "../headers/GameMode.h"
#include "../headers/GameStatistics.h"
#include "../headers/input/InputProviderForPlayerOne.h"
#include "../headers/input/InputProviderForPlayerTwo.h"
#include "../headers/obstacles/Brick.h"
#include "../headers/pawns/Bullet.h"
#include "../headers/pawns/Enemy.h"
#include "../headers/pawns/PlayerOne.h"
#include "../headers/pawns/PlayerTwo.h"

#include "gtest/gtest.h"

#include <memory>

class StatisticsTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events;
	std::shared_ptr<GameStatistics> _statistics;
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	UPoint _windowSize{.x = 800, .y = 600};
	float _tankSize{};
	float _tankSpeed{142};
	float _bulletSpeed{300.f};
	int* _windowBuffer{nullptr};
	int _tankHealth{100};
	std::shared_ptr<BulletPool> _bulletPool;

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_statistics = std::make_shared<GameStatistics>(_events);
		const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
		_tankSize = gridSize * 3;// for better turns
		const ObjRectangle playerRect{.x = 0, .y = 0, .w = _tankSize, .h = _tankSize};
		constexpr int yellow = 0xeaea00;
		std::string name = "Player";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		auto currentGameMode = OnePlayer;
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _windowSize, _windowBuffer, &currentGameMode);
		_allObjects.reserve(4);
		_allObjects.emplace_back(std::make_shared<PlayerOne>(playerRect, yellow, _tankHealth, _windowBuffer,
		                                                     _windowSize, UP, _tankSpeed, &_allObjects, _events,
		                                                     name, fraction, inputProvider, _bulletPool, false, 1));

		const ObjRectangle player2Rect{.x = _tankSize, .y = 0, .w = _tankSize, .h = _tankSize};
		constexpr int green = 0x408000;
		std::string name2 = "Player";
		std::string fraction2 = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider2 = std::make_unique<InputProviderForPlayerTwo>(name2, _events);
		_allObjects.emplace_back(std::make_shared<PlayerTwo>(player2Rect, green, _tankHealth, _windowBuffer,
		                                                     _windowSize, UP, _tankSpeed, &_allObjects, _events,
		                                                     name2, fraction2, inputProvider2, _bulletPool, false, true,
		                                                     2));
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
		player->SetPos({.x = 0.f, .y = 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		ObjRectangle bulletRect{.x = _tankSize / 2.f, .y = _tankSize, .w = bulletWidth, .h = bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Enemy1", "EnemyTeam", 0, false));
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
		player->SetPos({.x = 0.f, .y = 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		ObjRectangle bulletRect{.x = _tankSize / 2.f, .y = _tankSize, .w = bulletWidth, .h = bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Player2", "PlayerTeam", 0, false));
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
		player->SetPos({.x = 0.f, .y = 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		ObjRectangle bulletRect{.x = _tankSize / 2.f, .y = _tankSize, .w = bulletWidth, .h = bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Enemy1", "EnemyTeam", 0, false));
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
		player->SetPos({.x = 0.f, .y = 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		ObjRectangle bulletRect{.x = _tankSize / 2.f, .y = _tankSize, .w = bulletWidth, .h = bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Player1", "PlayerTeam", 0, false));
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

TEST_F(StatisticsTest, PlayerOneDiedByFriend)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		player->SetPos({.x = 0.f, .y = 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		ObjRectangle bulletRect{.x = 0, .y = _tankSize, .w = bulletWidth, .h = bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Player2", "PlayerTeam", 0, false));
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
		player->SetPos({.x = 0.f, .y = 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		ObjRectangle bulletRect{.x = _tankSize / 2.f, .y = _tankSize, .w = bulletWidth, .h = bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Enemy1", "EnemyTeam", 0, false));
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
		player->SetPos({.x = 0.f, .y = 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		ObjRectangle bulletRect{.x = _tankSize / 2.f, .y = _tankSize, .w = bulletWidth, .h = bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Enemy1", "EnemyTeam", 0, false));
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
		ObjRectangle bulletRect{.x = _tankSize + _tankSize / 2.f, .y = _tankSize, .w = bulletWidth, .h = bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Player1", "PlayerTeam", 0, false));
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
	const ObjRectangle enemy1Rect{.x = _tankSize * 2, .y = 0, .w = _tankSize, .h = _tankSize};
	constexpr int gray = 0x808080;
	_allObjects.emplace_back(std::make_shared<Enemy>(enemy1Rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN,
	                                                 _tankSpeed, &_allObjects, _events, "Enemy", "EnemyTeam",
	                                                 _bulletPool, false, 1));
	if (const auto enemy = dynamic_cast<Enemy*>(_allObjects.front().get()))
	{
		enemy->SetPos({.x = 0.f, .y = 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		ObjRectangle bulletRect{.x = _tankSize / 2.f, .y = _tankSize, .w = bulletWidth, .h = bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Enemy2", "EnemyTeam", 0, false));
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
	const ObjRectangle enemy1Rect{.x = _tankSize * 2, .y = 0, .w = _tankSize, .h = _tankSize};
	constexpr int gray = 0x808080;
	_allObjects.emplace_back(std::make_shared<Enemy>(enemy1Rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN,
	                                                 _tankSpeed, &_allObjects, _events, "Enemy", "EnemyTeam",
	                                                 _bulletPool, false, 1));
	if (const auto enemy = dynamic_cast<Enemy*>(_allObjects.front().get()))
	{
		enemy->SetPos({.x = 0.f, .y = 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		ObjRectangle bulletRect{.x = _tankSize / 2.f, .y = _tankSize, .w = bulletWidth, .h = bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Player1", "PlayerTeam", 0, false));
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
	const ObjRectangle enemy1Rect{.x = _tankSize * 2, .y = 0, .w = _tankSize, .h = _tankSize};
	constexpr int gray = 0x808080;
	_allObjects.emplace_back(std::make_shared<Enemy>(enemy1Rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN,
	                                                 _tankSpeed, &_allObjects, _events, "Enemy", "EnemyTeam",
	                                                 _bulletPool, false, 1));
	if (const auto enemy = dynamic_cast<Enemy*>(_allObjects.front().get()))
	{
		enemy->SetPos({.x = 0.f, .y = 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		ObjRectangle bulletRect{.x = _tankSize / 2.f, .y = _tankSize, .w = bulletWidth, .h = bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Player2", "PlayerTeam", 0, false));
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
	const ObjRectangle enemy1Rect{.x = _tankSize * 2, .y = 0, .w = _tankSize, .h = _tankSize};
	constexpr int gray = 0x808080;
	_allObjects.emplace_back(std::make_shared<Enemy>(enemy1Rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN,
	                                                 _tankSpeed, &_allObjects, _events, "Enemy", "EnemyTeam",
	                                                 _bulletPool, false, 1));
	if (const auto enemy = dynamic_cast<Enemy*>(_allObjects.front().get()))
	{
		enemy->SetPos({.x = 0.f, .y = 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		ObjRectangle bulletRect{.x = _tankSize / 2.f, .y = _tankSize, .w = bulletWidth, .h = bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Enemy2", "EnemyTeam", 0, false));
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
	const ObjRectangle enemy1Rect{.x = 0, .y = 0, .w = _tankSize, .h = _tankSize};
	constexpr int gray = 0x808080;
	_allObjects.emplace_back(std::make_shared<Enemy>(enemy1Rect, gray, _tankHealth, _windowBuffer, _windowSize,
	                                                 DOWN, _tankSpeed, &_allObjects, _events, "Enemy", "EnemyTeam",
	                                                 _bulletPool, false, 1));
	if (const auto enemy = dynamic_cast<Enemy*>(_allObjects.front().get()))
	{
		enemy->SetPos({.x = 0.f, .y = 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		ObjRectangle bulletRect{.x = 0, .y = _tankSize, .w = bulletWidth, .h = bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Player1", "PlayerTeam", 0, false));

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
	const ObjRectangle enemy1Rect{.x = _tankSize * 2, .y = 0, .w = _tankSize, .h = _tankSize};
	constexpr int gray = 0x808080;
	_allObjects.emplace_back(std::make_shared<Enemy>(enemy1Rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN,
	                                                 _tankSpeed, &_allObjects, _events, "Enemy", "EnemyTeam",
	                                                 _bulletPool, false, 1));
	if (const auto enemy = dynamic_cast<Enemy*>(_allObjects.front().get()))
	{
		enemy->SetPos({.x = 0.f, .y = 0.f});
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		ObjRectangle bulletRect{.x = _tankSize / 2.f, .y = _tankSize, .w = bulletWidth, .h = bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Player2", "PlayerTeam", 0, false));
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
	ObjRectangle bulletRect{.x = 0, .y = _tankSize, .w = bulletWidth, .h = bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, DOWN, _bulletSpeed, &_allObjects,
	                                                  _events, "Player1", "PlayerTeam", 0, false));
	if (const auto bullet1 = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Enemy1", "EnemyTeam", 1, false));
		if (const auto bullet2 = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			bullet2->SetPos({.x = 0, .y = _tankSize + bulletHeight + 1});
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
	ObjRectangle bulletRect{.x = 0, .y = _tankSize, .w = bulletWidth, .h = bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, DOWN, _bulletSpeed, &_allObjects,
	                                                  _events, "Player2", "PlayerTeam", 0, false));
	if (const auto bullet1 = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Player1", "PlayerTeam", 1, false));
		if (const auto bullet2 = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			bullet2->SetPos({.x = 0, .y = _tankSize + bulletHeight + 1});
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
	ObjRectangle bulletRect{.x = 0, .y = _tankSize, .w = bulletWidth, .h = bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, DOWN, _bulletSpeed, &_allObjects,
	                                                  _events, "Player1", "PlayerTeam", 0, false));
	if (const auto bullet1 = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Player2", "PlayerTeam", 1, false));
		if (const auto bullet2 = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			bullet2->SetPos({.x = 0, .y = _tankSize + bulletHeight + 1});
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
	const ObjRectangle bulletRect{.x = 0, .y = _tankSize + 1, .w = bulletWidth, .h = bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, direction, _bulletSpeed, &_allObjects,
	                                                  _events, "Enemy1", "EnemyTeam", 0, false));
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		constexpr float gridSize = 1;
		const ObjRectangle rect{.x = 0, .y = _tankSize + bulletHeight + 2 + gridSize, .w = gridSize, .h = gridSize};
		_allObjects.emplace_back(std::make_shared<Brick>(rect, _windowBuffer, _windowSize, _events, 0));
		if (const auto brick = dynamic_cast<Brick*>(_allObjects.back().get()))
		{
			brick->SetHealth(1);
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
	const ObjRectangle bulletRect{.x = 0, .y = _tankSize + 1, .w = bulletWidth, .h = bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, direction, _bulletSpeed, &_allObjects,
	                                                  _events, "Player1", "PlayerTeam", 0, false));
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		constexpr float gridSize = 1;
		const ObjRectangle rect{.x = 0, .y = _tankSize + bulletHeight + 2 + gridSize, .w = gridSize, .h = gridSize};
		_allObjects.emplace_back(std::make_shared<Brick>(rect, _windowBuffer, _windowSize, _events, 0));
		if (const auto brick = dynamic_cast<Brick*>(_allObjects.back().get()))
		{
			brick->SetHealth(1);
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
	const ObjRectangle bulletRect{.x = 0, .y = _tankSize, .w = bulletWidth, .h = bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, direction, _bulletSpeed, &_allObjects,
	                                                  _events, "Player2", "PlayerTeam", 0, false));
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		constexpr float gridSize = 1;
		const ObjRectangle rect{.x = 0, .y = _tankSize + bulletHeight + gridSize, .w = gridSize, .h = gridSize};
		_allObjects.emplace_back(std::make_shared<Brick>(rect, _windowBuffer, _windowSize, _events, 0));
		if (const auto brick = dynamic_cast<Brick*>(_allObjects.back().get()))
		{
			brick->SetHealth(1);
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
	const ObjRectangle bulletRect{.x = 0, .y = _tankSize, .w = bulletWidth, .h = bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, DOWN, _bulletSpeed, &_allObjects,
	                                                  _events, "Enemy1", "EnemyTeam", 0, false));
	if (const auto bullet1 = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Enemy2", "EnemyTeam", 1, false));
		if (const auto bullet2 = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			bullet2->SetPos({.x = 0, .y = _tankSize + bulletHeight + 1});
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
	const ObjRectangle bulletRect{.x = 0, .y = _tankSize, .w = bulletWidth, .h = bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, DOWN, _bulletSpeed, &_allObjects,
	                                                  _events, "Player1", "PlayerTeam", 0, false));
	if (const auto bullet1 = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Player2", "PlayerTeam", 1, false));
		if (const auto bullet2 = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			bullet2->SetPos({.x = 0, .y = _tankSize + bulletHeight + 1});
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
	const ObjRectangle bulletRect{.x = 0, .y = _tankSize, .w = bulletWidth, .h = bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, DOWN, _bulletSpeed,
	                                                  &_allObjects,
	                                                  _events, "Player1", "PlayerTeam", 0, false));
	if (const auto bullet1 = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed,
		                                                  &_allObjects, _events, "Enemy1", "EnemyTeam", 1, false));
		if (const auto bullet2 = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			bullet2->SetPos({.x = 0, .y = _tankSize + bulletHeight + 1});
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
	const ObjRectangle bulletRect{.x = 0, .y = _tankSize, .w = bulletWidth, .h = bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, DOWN, _bulletSpeed, &_allObjects,
	                                                  _events, "Player2", "PlayerTeam", 0, false));
	if (const auto bullet1 = dynamic_cast<Bullet*>(_allObjects.front().get()))
	{
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, UP, _bulletSpeed, &_allObjects,
		                                                  _events, "Enemy1", "EnemyTeam", 1, false));
		if (const auto bullet2 = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			bullet2->SetPos({.x = 0, .y = _tankSize + bulletHeight + 1});
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
