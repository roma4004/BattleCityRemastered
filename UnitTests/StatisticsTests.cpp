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

class StatisticsTest : public ::testing::Test
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
		_allObjects.emplace_back(std::make_shared<MockPlayerOne>(playerRect, yellow, _tankHealth, _windowBuffer,
		                                                         _windowSize, UP, _tankSpeed, &_allObjects, _events,
		                                                         name, fraction, inputProvider, _bulletPool));

		const Rectangle player2Rect{_tankSize, 0, _tankSize, _tankSize};
		constexpr int green = 0x408000;
		auto name2 = "PlayerTwo";
		auto fraction2 = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider2 = std::make_unique<InputProviderForPlayerTwo>(name2, _events);
		_allObjects.emplace_back(std::make_shared<PlayerTwo>(player2Rect, green, _tankHealth, _windowBuffer,
		                                                     _windowSize, UP, _tankSpeed, &_allObjects, _events, name2,
		                                                     fraction2, inputProvider2,
		                                                     _bulletPool));
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

TEST_F(StatisticsTest, PlayerOneHitByEnemy)
{
	if (const auto player = dynamic_cast<MockPlayerOne*>(_allObjects.front().get()))
	{
		player->SetPos({0.f, 0.f});
		constexpr Direction direction = Direction::UP;
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, direction, _bulletSpeed,
		                                                  &_allObjects, _events, "Enemy1", "EnemyTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			EXPECT_EQ(_statistics->playerOneHitByEnemyTeam, 0);
			constexpr float deltaTime = 1.f / 60.f;
			bullet->TickUpdate(deltaTime);
			EXPECT_EQ(_statistics->playerOneHitByEnemyTeam, 1);
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

TEST_F(StatisticsTest, PlayerOneHitByFriend)
{
	if (const auto player = dynamic_cast<MockPlayerOne*>(_allObjects.front().get()))
	{
		player->SetPos({0.f, 0.f});
		constexpr Direction direction = Direction::UP;
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, direction, _bulletSpeed,
		                                                  &_allObjects, _events, "PlayerTwo", "PlayerTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			EXPECT_EQ(_statistics->playerOneHitFriendlyFire, 0);
			constexpr float deltaTime = 1.f / 60.f;
			bullet->TickUpdate(deltaTime);
			EXPECT_EQ(_statistics->playerOneHitFriendlyFire, 1);
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

TEST_F(StatisticsTest, PlayerTwoHitByEnemy)
{
	if (const auto player = dynamic_cast<PlayerTwo*>(_allObjects.back().get()))
	{
		player->SetPos({0.f, 0.f});
		constexpr Direction direction = Direction::UP;
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, direction, _bulletSpeed,
		                                                  &_allObjects, _events, "Enemy1", "EnemyTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			EXPECT_EQ(_statistics->playerTwoHitByEnemyTeam, 0);
			constexpr float deltaTime = 1.f / 60.f;
			bullet->TickUpdate(deltaTime);
			EXPECT_EQ(_statistics->playerTwoHitByEnemyTeam, 1);
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

TEST_F(StatisticsTest, PlayerTwoHitByFriend)
{
	if (const auto player = dynamic_cast<PlayerTwo*>(_allObjects.back().get()))
	{
		player->SetPos({0.f, 0.f});
		constexpr Direction direction = Direction::UP;
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, direction, _bulletSpeed,
		                                                  &_allObjects, _events, "PlayerOne", "PlayerTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			EXPECT_EQ(_statistics->playerTwoHitFriendlyFire, 0);
			constexpr float deltaTime = 1.f / 60.f;
			bullet->TickUpdate(deltaTime);
			EXPECT_EQ(_statistics->playerTwoHitFriendlyFire, 1);
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

TEST_F(StatisticsTest, EnemyOneRespawnNeededFlag)
{
	EXPECT_EQ(_statistics->enemyOneNeedRespawn, false);
	{
		const Rectangle rect{_tankSize * 2, 0, _tankSize, _tankSize};
		constexpr int gray = 0x808080;
		std::string name = "Enemy1";
		std::string fraction = "EnemyTeam";
		auto enemy = std::make_unique<Enemy>(rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN, _tankSpeed,
		                                     &_allObjects, _events, name, fraction, _bulletPool);
	}
	EXPECT_EQ(_statistics->enemyOneNeedRespawn, true);
}

TEST_F(StatisticsTest, EnemyTwoRespawnNeededFlag)
{
	EXPECT_EQ(_statistics->enemyTwoNeedRespawn, false);
	{
		const Rectangle rect{_tankSize * 2, 0, _tankSize, _tankSize};
		constexpr int gray = 0x808080;
		std::string name = "Enemy2";
		std::string fraction = "EnemyTeam";
		auto enemy = std::make_unique<Enemy>(rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN, _tankSpeed,
		                                     &_allObjects, _events, name, fraction, _bulletPool);
	}
	EXPECT_EQ(_statistics->enemyTwoNeedRespawn, true);
}

TEST_F(StatisticsTest, EnemyThreeRespawnNeededFlag)
{
	EXPECT_EQ(_statistics->enemyThreeNeedRespawn, false);
	{
		const Rectangle rect{_tankSize * 2, 0, _tankSize, _tankSize};
		constexpr int gray = 0x808080;
		std::string name = "Enemy3";
		std::string fraction = "EnemyTeam";
		auto enemy = std::make_unique<Enemy>(rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN, _tankSpeed,
		                                     &_allObjects, _events, name, fraction, _bulletPool);
	}
	EXPECT_EQ(_statistics->enemyThreeNeedRespawn, true);
}

TEST_F(StatisticsTest, EnemyFourRespawnNeededFlag)
{
	EXPECT_EQ(_statistics->enemyFourNeedRespawn, false);
	{
		const Rectangle rect{_tankSize * 2, 0, _tankSize, _tankSize};
		constexpr int gray = 0x808080;
		std::string name = "Enemy4";
		std::string fraction = "EnemyTeam";
		auto enemy = std::make_unique<Enemy>(rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN, _tankSpeed,
		                                     &_allObjects, _events, name, fraction, _bulletPool);
	}
	EXPECT_EQ(_statistics->enemyFourNeedRespawn, true);
}

TEST_F(StatisticsTest, PlayerOneDiedRespawnNeededFlag)
{
	EXPECT_EQ(_statistics->playerOneNeedRespawn, false);
	{
		const Rectangle rect{_tankSize * 2, 0, _tankSize, _tankSize};
		constexpr int yellow = 0xeaea00;
		std::string name = "PlayerOne";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		auto enemy = std::make_shared<PlayerOne>(rect, yellow, _tankHealth, _windowBuffer, _windowSize, UP, _tankSpeed,
		                                         &_allObjects, _events, name, fraction, inputProvider, _bulletPool);
	}
	EXPECT_EQ(_statistics->playerOneNeedRespawn, true);
}

TEST_F(StatisticsTest, PlayerTwoDiedRespawnNeededFlag)
{
	EXPECT_EQ(_statistics->playerTwoNeedRespawn, false);
	{
		const Rectangle rect{_tankSize * 2, 0, _tankSize, _tankSize};
		constexpr int green = 0x408000;
		std::string name = "PlayerTwo";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		auto enemy = std::make_shared<PlayerTwo>(rect, green, _tankHealth, _windowBuffer, _windowSize, UP, _tankSpeed,
		                                         &_allObjects, _events, name, fraction, inputProvider, _bulletPool);
	}
	EXPECT_EQ(_statistics->playerTwoNeedRespawn, true);
}

TEST_F(StatisticsTest, EnemyDiedRespawnCount)
{
	const auto respawnResource = _statistics->enemyRespawnResource;
	{
		const Rectangle rect{_tankSize * 2, 0, _tankSize, _tankSize};
		constexpr int gray = 0x808080;
		std::string name = "Enemy1";
		std::string fraction = "EnemyTeam";
		auto enemy = std::make_unique<Enemy>(rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN, _tankSpeed,
		                                     &_allObjects, _events, name, fraction, _bulletPool);
	}
	EXPECT_GT(respawnResource, _statistics->enemyRespawnResource);
}

TEST_F(StatisticsTest, PlayerOneDiedRespawnCount)
{
	const auto respawnResource = _statistics->playerOneRespawnResource;
	{
		const Rectangle rect{_tankSize * 2, 0, _tankSize, _tankSize};
		constexpr int yellow = 0xeaea00;
		std::string name = "PlayerOne";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		auto enemy = std::make_shared<PlayerOne>(rect, yellow, _tankHealth, _windowBuffer, _windowSize, UP, _tankSpeed,
		                                         &_allObjects, _events, name, fraction, inputProvider, _bulletPool);
	}
	EXPECT_GT(respawnResource, _statistics->playerOneRespawnResource);
}

TEST_F(StatisticsTest, PlayerTwoDiedRespawnCount)
{
	const auto respawnResource = _statistics->playerTwoRespawnResource;
	{
		const Rectangle rect{_tankSize * 2, 0, _tankSize, _tankSize};
		constexpr int green = 0x408000;
		std::string name = "PlayerTwo";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		auto enemy = std::make_shared<PlayerTwo>(rect, green, _tankHealth, _windowBuffer, _windowSize, UP, _tankSpeed,
		                                         &_allObjects, _events, name, fraction, inputProvider, _bulletPool);
	}
	EXPECT_GT(respawnResource, _statistics->playerTwoRespawnResource);
}

TEST_F(StatisticsTest, PlayerOneDiedByFriend)
{
	if (const auto player = dynamic_cast<PlayerTwo*>(_allObjects.back().get()))
	{
		player->SetPos({0.f, 0.f});
		constexpr Direction direction = Direction::UP;
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, direction, _bulletSpeed,
		                                                  &_allObjects, _events, "PlayerTwo", "PlayerTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			player->SetHealth(1);
			EXPECT_EQ(_statistics->playerOneDiedByFriendlyFire, 0);
			constexpr float deltaTime = 1.f / 60.f;
			bullet->TickUpdate(deltaTime);
			EXPECT_EQ(_statistics->playerOneDiedByFriendlyFire, 1);
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

TEST_F(StatisticsTest, PlayerTwoDiedByEnemy)
{
	if (const auto player = dynamic_cast<PlayerTwo*>(_allObjects.back().get()))
	{
		player->SetPos({0.f, 0.f});
		constexpr Direction direction = Direction::UP;
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, direction, _bulletSpeed,
		                                                  &_allObjects, _events, "Enemy1", "EnemyTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			player->SetHealth(1);
			EXPECT_EQ(_statistics->playerDiedByEnemyTeam, 0);
			constexpr float deltaTime = 1.f / 60.f;
			bullet->TickUpdate(deltaTime);
			EXPECT_EQ(_statistics->playerDiedByEnemyTeam, 1);
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

TEST_F(StatisticsTest, PlayerOneDiedByEnemy)
{
	if (const auto player = dynamic_cast<PlayerOne*>(_allObjects.front().get()))
	{
		player->SetPos({0.f, 0.f});
		constexpr Direction direction = Direction::UP;
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, direction, _bulletSpeed,
		                                                  &_allObjects, _events, "Enemy1", "EnemyTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			player->SetHealth(1);
			EXPECT_EQ(_statistics->playerDiedByEnemyTeam, 0);
			constexpr float deltaTime = 1.f / 60.f;
			bullet->TickUpdate(deltaTime);
			EXPECT_EQ(_statistics->playerDiedByEnemyTeam, 1);
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

TEST_F(StatisticsTest, PlayerTwoDiedByFriend)
{
	if (const auto player = dynamic_cast<PlayerTwo*>(_allObjects.back().get()))
	{
		player->SetPos({0.f, 0.f});
		constexpr Direction direction = Direction::UP;
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, direction, _bulletSpeed,
		                                                  &_allObjects, _events, "PlayerOne", "PlayerTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			player->SetHealth(1);
			EXPECT_EQ(_statistics->playerTwoDiedByFriendlyFire, 0);
			constexpr float deltaTime = 1.f / 60.f;
			bullet->TickUpdate(deltaTime);
			EXPECT_EQ(_statistics->playerTwoDiedByFriendlyFire, 1);
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

TEST_F(StatisticsTest, EnemyHitByFriend)
{
	const Rectangle enemy1Rect{_tankSize * 2, 0, _tankSize, _tankSize};
	constexpr int gray = 0x808080;
	const auto indexString = std::to_string(1);
	_allObjects.emplace_back(std::make_shared<Enemy>(enemy1Rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN,
	                                                 _tankSpeed, &_allObjects, _events, "Enemy" + indexString,
	                                                 "EnemyTeam", _bulletPool));
	if (const auto enemy = dynamic_cast<Enemy*>(_allObjects.back().get()))
	{
		enemy->SetPos({0.f, 0.f});
		constexpr Direction direction = Direction::UP;
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, direction, _bulletSpeed,
		                                                  &_allObjects, _events, "Enemy2", "EnemyTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			EXPECT_EQ(_statistics->enemyHitByFriendlyFire, 0);
			constexpr float deltaTime = 1.f / 60.f;
			bullet->TickUpdate(deltaTime);
			EXPECT_EQ(_statistics->enemyHitByFriendlyFire, 1);
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

TEST_F(StatisticsTest, EnemyHitByPlayerOne)
{
	const Rectangle enemy1Rect{_tankSize * 2, 0, _tankSize, _tankSize};
	constexpr int gray = 0x808080;
	const auto indexString = std::to_string(1);
	_allObjects.emplace_back(std::make_shared<Enemy>(enemy1Rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN,
	                                                 _tankSpeed, &_allObjects, _events, "Enemy" + indexString,
	                                                 "EnemyTeam", _bulletPool));
	if (const auto enemy = dynamic_cast<Enemy*>(_allObjects.back().get()))
	{
		enemy->SetPos({0.f, 0.f});
		constexpr Direction direction = Direction::UP;
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, direction, _bulletSpeed,
		                                                  &_allObjects, _events, "PlayerOne", "PlayerTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			EXPECT_EQ(_statistics->enemyHitByPlayerOne, 0);
			constexpr float deltaTime = 1.f / 60.f;
			bullet->TickUpdate(deltaTime);
			EXPECT_EQ(_statistics->enemyHitByPlayerOne, 1);
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

TEST_F(StatisticsTest, EnemyHitByPlayerTwo)
{
	const Rectangle enemy1Rect{_tankSize * 2, 0, _tankSize, _tankSize};
	constexpr int gray = 0x808080;
	const auto indexString = std::to_string(1);
	_allObjects.emplace_back(std::make_shared<Enemy>(enemy1Rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN,
	                                                 _tankSpeed, &_allObjects, _events, "Enemy" + indexString,
	                                                 "EnemyTeam", _bulletPool));
	if (const auto enemy = dynamic_cast<Enemy*>(_allObjects.back().get()))
	{
		enemy->SetPos({0.f, 0.f});
		constexpr Direction direction = Direction::UP;
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, direction, _bulletSpeed,
		                                                  &_allObjects, _events, "PlayerTwo", "PlayerTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			EXPECT_EQ(_statistics->enemyHitByPlayerTwo, 0);
			constexpr float deltaTime = 1.f / 60.f;
			bullet->TickUpdate(deltaTime);
			EXPECT_EQ(_statistics->enemyHitByPlayerTwo, 1);
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

TEST_F(StatisticsTest, EnemyDiedByFriend)
{
	const Rectangle enemy1Rect{_tankSize * 2, 0, _tankSize, _tankSize};
	constexpr int gray = 0x808080;
	const auto indexString = std::to_string(1);
	_allObjects.emplace_back(std::make_shared<Enemy>(enemy1Rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN,
	                                                 _tankSpeed, &_allObjects, _events, "Enemy" + indexString,
	                                                 "EnemyTeam", _bulletPool));
	if (const auto enemy = dynamic_cast<Enemy*>(_allObjects.back().get()))
	{
		enemy->SetPos({0.f, 0.f});
		constexpr Direction direction = Direction::UP;
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, direction, _bulletSpeed,
		                                                  &_allObjects, _events, "Enemy2", "EnemyTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			enemy->SetHealth(1);
			EXPECT_EQ(_statistics->enemyDiedByFriendlyFire, 0);
			constexpr float deltaTime = 1.f / 60.f;
			bullet->TickUpdate(deltaTime);
			EXPECT_EQ(_statistics->enemyDiedByFriendlyFire, 1);
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

TEST_F(StatisticsTest, EnemyDiedByPlayerOne)
{
	const Rectangle enemy1Rect{_tankSize * 2, 0, _tankSize, _tankSize};
	constexpr int gray = 0x808080;
	const auto indexString = std::to_string(1);
	_allObjects.emplace_back(std::make_shared<Enemy>(enemy1Rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN,
	                                                 _tankSpeed, &_allObjects, _events, "Enemy" + indexString,
	                                                 "EnemyTeam", _bulletPool));
	if (const auto enemy = dynamic_cast<Enemy*>(_allObjects.back().get()))
	{
		enemy->SetPos({0.f, 0.f});
		constexpr Direction direction = Direction::UP;
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, direction, _bulletSpeed,
		                                                  &_allObjects, _events, "PlayerOne", "PlayerTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			enemy->SetHealth(1);
			EXPECT_EQ(_statistics->enemyDiedByPlayerOne, 0);
			constexpr float deltaTime = 1.f / 60.f;
			bullet->TickUpdate(deltaTime);
			EXPECT_EQ(_statistics->enemyDiedByPlayerOne, 1);
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

TEST_F(StatisticsTest, EnemyDiedByPlayerTwo)
{
	const Rectangle enemy1Rect{_tankSize * 2, 0, _tankSize, _tankSize};
	constexpr int gray = 0x808080;
	const auto indexString = std::to_string(1);
	_allObjects.emplace_back(std::make_shared<Enemy>(enemy1Rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN,
	                                                 _tankSpeed, &_allObjects, _events, "Enemy" + indexString,
	                                                 "EnemyTeam", _bulletPool));
	if (const auto enemy = dynamic_cast<Enemy*>(_allObjects.back().get()))
	{
		enemy->SetPos({0.f, 0.f});
		constexpr Direction direction = Direction::UP;
		constexpr float bulletWidth = 6;
		constexpr float bulletHeight = 5;
		Rectangle bulletRect{_tankSize / 2.f, _tankSize, bulletWidth, bulletHeight};
		constexpr int color = 0xffffff;
		constexpr int health = 1;
		constexpr int damage = 1;
		constexpr double bulletDamageAreaRadius = 12.0;
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, direction, _bulletSpeed,
		                                                  &_allObjects, _events, "PlayerTwo", "PlayerTeam"));
		if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			enemy->SetHealth(1);
			EXPECT_EQ(_statistics->enemyDiedByPlayerTwo, 0);
			constexpr float deltaTime = 1.f / 60.f;
			bullet->TickUpdate(deltaTime);
			EXPECT_EQ(_statistics->enemyDiedByPlayerTwo, 1);
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

TEST_F(StatisticsTest, BulletHitByEnemy)
{
	constexpr Direction direction = Direction::UP;
	constexpr float bulletWidth = 6;
	constexpr float bulletHeight = 5;
	Rectangle bulletRect{0, _tankSize, bulletWidth, bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, direction, _bulletSpeed, &_allObjects,
	                                                  _events, "PlayerOne", "PlayerTeam"));
	if (const auto bullet1 = dynamic_cast<Bullet*>(_allObjects.back().get()))
	{
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, direction, _bulletSpeed,
		                                                  &_allObjects, _events, "Enemy1", "EnemyTeam"));
		if (const auto bullet2 = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			bullet2->SetPos({0, _tankSize + bulletHeight + 1});
			EXPECT_EQ(_statistics->bulletHitByEnemy, 0);
			constexpr float deltaTime = 1.f / 60.f;
			bullet2->TickUpdate(deltaTime);
			EXPECT_EQ(_statistics->bulletHitByEnemy, 1);
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

TEST_F(StatisticsTest, BulletHitByPlayerOne)
{
	constexpr Direction direction = Direction::UP;
	constexpr float bulletWidth = 6;
	constexpr float bulletHeight = 5;
	Rectangle bulletRect{0, _tankSize, bulletWidth, bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, direction, _bulletSpeed, &_allObjects,
	                                                  _events, "PlayerTwo", "PlayerTeam"));
	if (const auto bullet1 = dynamic_cast<Bullet*>(_allObjects.back().get()))
	{
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, direction, _bulletSpeed,
		                                                  &_allObjects, _events, "PlayerOne", "PlayerTeam"));
		if (const auto bullet2 = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			bullet2->SetPos({0, _tankSize + bulletHeight + 1});
			EXPECT_EQ(_statistics->bulletHitByPlayerOne, 0);
			constexpr float deltaTime = 1.f / 60.f;
			bullet2->TickUpdate(deltaTime);
			EXPECT_EQ(_statistics->bulletHitByPlayerOne, 1);
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

TEST_F(StatisticsTest, BulletHitByPlayerTwo)
{
	constexpr Direction direction = Direction::UP;
	constexpr float bulletWidth = 6;
	constexpr float bulletHeight = 5;
	Rectangle bulletRect{0, _tankSize, bulletWidth, bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, direction, _bulletSpeed, &_allObjects,
	                                                  _events, "PlayerOne", "PlayerTeam"));
	if (const auto bullet1 = dynamic_cast<Bullet*>(_allObjects.back().get()))
	{
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, direction, _bulletSpeed,
		                                                  &_allObjects, _events, "PlayerTwo", "PlayerTeam"));
		if (const auto bullet2 = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			bullet2->SetPos({0, _tankSize + bulletHeight + 1});
			EXPECT_EQ(_statistics->bulletHitByPlayerTwo, 0);
			constexpr float deltaTime = 1.f / 60.f;
			bullet2->TickUpdate(deltaTime);
			EXPECT_EQ(_statistics->bulletHitByPlayerTwo, 1);
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

TEST_F(StatisticsTest, BrickDiedByEnemy)
{
	constexpr Direction direction = Direction::DOWN;
	constexpr float bulletWidth = 6;
	constexpr float bulletHeight = 5;
	Rectangle bulletRect{0, _tankSize + 1, bulletWidth, bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, direction, _bulletSpeed, &_allObjects,
	                                                  _events, "Enemy1", "EnemyTeam"));
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
	{
		float _gridSize = 1;
		const Rectangle rect{0, _tankSize + bulletHeight + 2 + _gridSize, _gridSize, _gridSize};
		_allObjects.emplace_back(std::make_shared<Brick>(rect, _windowBuffer, _windowSize, _events));
		if (const auto brick = dynamic_cast<Brick*>(_allObjects.back().get()))
		{
			EXPECT_EQ(_statistics->brickDiedByEnemyTeam, 0);
			constexpr float deltaTime = 1.f / 60.f;
			bullet->TickUpdate(deltaTime);
			EXPECT_EQ(_statistics->brickDiedByEnemyTeam, 1);
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

TEST_F(StatisticsTest, BrickDiedByPlayerOne)
{
	constexpr Direction direction = Direction::DOWN;
	constexpr float bulletWidth = 6;
	constexpr float bulletHeight = 5;
	Rectangle bulletRect{0, _tankSize + 1, bulletWidth, bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, direction, _bulletSpeed, &_allObjects,
	                                                  _events, "PlayerOne", "PlayerTeam"));
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
	{
		float _gridSize = 1;
		const Rectangle rect{0, _tankSize + bulletHeight + 2 + _gridSize, _gridSize, _gridSize};
		_allObjects.emplace_back(std::make_shared<Brick>(rect, _windowBuffer, _windowSize, _events));
		if (const auto brick = dynamic_cast<Brick*>(_allObjects.back().get()))
		{
			EXPECT_EQ(_statistics->brickDiedByPlayerOne, 0);
			constexpr float deltaTime = 1.f / 60.f;
			bullet->TickUpdate(deltaTime);
			EXPECT_EQ(_statistics->brickDiedByPlayerOne, 1);
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

TEST_F(StatisticsTest, BrickDiedByPlayerTwo)
{
	constexpr Direction direction = Direction::DOWN;
	constexpr float bulletWidth = 6;
	constexpr float bulletHeight = 5;
	Rectangle bulletRect{0, _tankSize, bulletWidth, bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, direction, _bulletSpeed, &_allObjects,
	                                                  _events, "PlayerTwo", "PlayerTeam"));
	if (const auto bullet = dynamic_cast<Bullet*>(_allObjects.back().get()))
	{
		float _gridSize = 1;
		const Rectangle rect{0, _tankSize + bulletHeight + _gridSize, _gridSize, _gridSize};
		_allObjects.emplace_back(std::make_shared<Brick>(rect, _windowBuffer, _windowSize, _events));
		if (const auto brick = dynamic_cast<Brick*>(_allObjects.back().get()))
		{
			EXPECT_EQ(_statistics->brickDiedByPlayerTwo, 0);
			constexpr float deltaTime = 1.f / 60.f;
			bullet->TickUpdate(deltaTime);
			EXPECT_EQ(_statistics->brickDiedByPlayerTwo, 1);
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

TEST_F(StatisticsTest, BulletHitBulletByEnemyAndByEnemy)
{
	constexpr Direction direction = Direction::UP;
	constexpr float bulletWidth = 6;
	constexpr float bulletHeight = 5;
	Rectangle bulletRect{0, _tankSize, bulletWidth, bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, direction, _bulletSpeed, &_allObjects,
	                                                  _events, "Enemy1", "EnemyTeam"));
	if (const auto bullet1 = dynamic_cast<Bullet*>(_allObjects.back().get()))
	{
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, direction, _bulletSpeed,
		                                                  &_allObjects, _events, "Enemy2", "EnemyTeam"));
		if (const auto bullet2 = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			bullet2->SetPos({0, _tankSize + bulletHeight + 1});
			EXPECT_EQ(_statistics->bulletHitByEnemy, 0);
			constexpr float deltaTime = 1.f / 60.f;
			bullet2->TickUpdate(deltaTime);
			EXPECT_EQ(_statistics->bulletHitByEnemy, 2);
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

TEST_F(StatisticsTest, BulletHitBulletPlayerOneAndByPlayerTwo)
{
	constexpr Direction direction = Direction::UP;
	constexpr float bulletWidth = 6;
	constexpr float bulletHeight = 5;
	Rectangle bulletRect{0, _tankSize, bulletWidth, bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, direction, _bulletSpeed, &_allObjects,
	                                                  _events, "PlayerOne", "PlayerTeam"));
	if (const auto bullet1 = dynamic_cast<Bullet*>(_allObjects.back().get()))
	{
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, direction, _bulletSpeed,
		                                                  &_allObjects, _events, "PlayerTwo", "PlayerTeam"));
		if (const auto bullet2 = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			bullet2->SetPos({0, _tankSize + bulletHeight + 1});
			EXPECT_EQ(_statistics->bulletHitByPlayerOne, 0);
			EXPECT_EQ(_statistics->bulletHitByPlayerTwo, 0);
			constexpr float deltaTime = 1.f / 60.f;
			bullet2->TickUpdate(deltaTime);
			EXPECT_EQ(_statistics->bulletHitByPlayerOne, 1);
			EXPECT_EQ(_statistics->bulletHitByPlayerTwo, 1);
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

TEST_F(StatisticsTest, BulletHitBulletByEnemyAndByPlayerTwo)
{
	constexpr Direction direction = Direction::UP;
	constexpr float bulletWidth = 6;
	constexpr float bulletHeight = 5;
	Rectangle bulletRect{0, _tankSize, bulletWidth, bulletHeight};
	constexpr int color = 0xffffff;
	constexpr int health = 1;
	constexpr int damage = 1;
	constexpr double bulletDamageAreaRadius = 12.0;
	_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
	                                                  _windowBuffer, _windowSize, direction, _bulletSpeed, &_allObjects,
	                                                  _events, "PlayerTwo", "PlayerTeam"));
	if (const auto bullet1 = dynamic_cast<Bullet*>(_allObjects.back().get()))
	{
		_allObjects.emplace_back(std::make_shared<Bullet>(bulletRect, damage, bulletDamageAreaRadius, color, health,
		                                                  _windowBuffer, _windowSize, direction, _bulletSpeed,
		                                                  &_allObjects, _events, "Enemy1", "EnemyTeam"));
		if (const auto bullet2 = dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			bullet2->SetPos({0, _tankSize + bulletHeight + 1});
			EXPECT_EQ(_statistics->bulletHitByEnemy, 0);
			EXPECT_EQ(_statistics->bulletHitByPlayerTwo, 0);
			constexpr float deltaTime = 1.f / 60.f;
			bullet2->TickUpdate(deltaTime);
			EXPECT_EQ(_statistics->bulletHitByEnemy, 1);
			EXPECT_EQ(_statistics->bulletHitByPlayerTwo, 1);
		}
		else
		{
			EXPECT_TRUE(false);
		}
	}
	else
	{
		EXPECT_TRUE(false);
	}
}
