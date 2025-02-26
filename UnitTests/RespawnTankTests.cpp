#include "../headers/EventSystem.h"
#include "../headers/GameMode.h"
#include "../headers/GameStatistics.h"
#include "../headers/TankSpawner.h"
#include "../headers/input/InputProviderForPlayerOne.h"
#include "../headers/input/InputProviderForPlayerTwo.h"
#include "../headers/pawns/Enemy.h"
#include "../headers/pawns/PlayerOne.h"
#include "../headers/pawns/PlayerTwo.h"

#include "gtest/gtest.h"

#include <memory>

class TankSpawnerTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<GameStatistics> _statistics{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<int[]> _windowBuffer{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	UPoint _windowSize{.x = 800, .y = 600};
	int _tankHealth{100};
	float _tankSize{};
	float _tankSpeed{142};
	float _bulletSpeed{300.f};
	int _yellow{0xeaea00};
	int _green{0x408000};
	int _gray{0x808080};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_statistics = std::make_shared<GameStatistics>(_events);
		GameMode currentMode = OnePlayer;
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _windowSize, _windowBuffer, currentMode);
		_tankSpawner = std::make_shared<TankSpawner>(_windowSize, _windowBuffer, &_allObjects, _events, _bulletPool);
		const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
		_tankSize = gridSize * 3;// for better turns
		const ObjRectangle playerRect{.x = 0, .y = 0, .w = _tankSize, .h = _tankSize};
		constexpr int yellow = 0xeaea00;
		std::string name = "Player";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		auto currentGameMode = OnePlayer;
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _windowSize, _windowBuffer, currentGameMode);
		_allObjects.reserve(4);
		_allObjects.emplace_back(
				std::make_shared<PlayerOne>(
						playerRect, yellow, _tankHealth, _windowBuffer, _windowSize, UP, _tankSpeed, &_allObjects,
						_events, name, fraction, std::move(inputProvider), _bulletPool, false, 1));

		const ObjRectangle player2Rect{.x = _tankSize, .y = 0, .w = _tankSize, .h = _tankSize};
		constexpr int green = 0x408000;
		std::string name2 = "Player";
		std::string fraction2 = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider2 = std::make_unique<InputProviderForPlayerTwo>(name2, _events);
		_allObjects.emplace_back(
				std::make_shared<PlayerTwo>(
						player2Rect, green, _tankHealth, _windowBuffer, _windowSize, UP, _tankSpeed, &_allObjects,
						_events, name2, fraction2, std::move(inputProvider2), _bulletPool, false, true, 2));
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

TEST_F(TankSpawnerTest, EnemyOneRespawnNeededFlag)
{
	EXPECT_EQ(_tankSpawner->IsEnemyOneNeedRespawn(), false);
	{
		const ObjRectangle rect{.x = _tankSize * 2, .y = 0, .w = _tankSize, .h = _tankSize};
		auto enemy = std::make_unique<Enemy>(
				rect, _gray, _tankHealth, _windowBuffer, _windowSize, DOWN, _tankSpeed, &_allObjects, _events, "Enemy",
				"EnemyTeam", _bulletPool, false, 1);
	}
	EXPECT_EQ(_tankSpawner->IsEnemyOneNeedRespawn(), true);
}

TEST_F(TankSpawnerTest, EnemyTwoRespawnNeededFlag)
{
	EXPECT_EQ(_tankSpawner->IsEnemyTwoNeedRespawn(), false);
	{
		const ObjRectangle rect{.x = _tankSize * 2, .y = 0, .w = _tankSize, .h = _tankSize};
		auto enemy = std::make_unique<Enemy>(
				rect, _gray, _tankHealth, _windowBuffer, _windowSize, DOWN, _tankSpeed, &_allObjects, _events, "Enemy",
				"EnemyTeam", _bulletPool, false, 2);
	}
	EXPECT_EQ(_tankSpawner->IsEnemyTwoNeedRespawn(), true);
}

TEST_F(TankSpawnerTest, EnemyThreeRespawnNeededFlag)
{
	EXPECT_EQ(_tankSpawner->IsEnemyThreeNeedRespawn(), false);
	{
		const ObjRectangle rect{.x = _tankSize * 2, .y = 0, .w = _tankSize, .h = _tankSize};
		auto enemy = std::make_unique<Enemy>(
				rect, _gray, _tankHealth, _windowBuffer, _windowSize, DOWN, _tankSpeed, &_allObjects, _events, "Enemy",
				"EnemyTeam", _bulletPool, false, 3);
	}
	EXPECT_EQ(_tankSpawner->IsEnemyThreeNeedRespawn(), true);
}

TEST_F(TankSpawnerTest, EnemyFourRespawnNeededFlag)
{
	EXPECT_EQ(_tankSpawner->IsEnemyFourNeedRespawn(), false);
	{
		const ObjRectangle rect{.x = _tankSize * 2, .y = 0, .w = _tankSize, .h = _tankSize};
		auto enemy = std::make_unique<Enemy>(
				rect, _gray, _tankHealth, _windowBuffer, _windowSize, DOWN, _tankSpeed, &_allObjects, _events, "Enemy",
				"EnemyTeam", _bulletPool, false, 4);
	}
	EXPECT_EQ(_tankSpawner->IsEnemyFourNeedRespawn(), true);
}

TEST_F(TankSpawnerTest, PlayerOneDiedRespawnNeededFlag)
{
	EXPECT_EQ(_tankSpawner->IsPlayerOneNeedRespawn(), false);
	{
		const ObjRectangle rect{.x = _tankSize * 2, .y = 0, .w = _tankSize, .h = _tankSize};
		std::string name = "Player";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		auto enemy = std::make_shared<PlayerOne>(
				rect, _yellow, _tankHealth, _windowBuffer, _windowSize, UP, _tankSpeed, &_allObjects, _events, name,
				fraction, std::move(inputProvider), _bulletPool, false, 1);
	}
	EXPECT_EQ(_tankSpawner->IsPlayerOneNeedRespawn(), true);
}

TEST_F(TankSpawnerTest, PlayerTwoDiedRespawnNeededFlag)
{
	EXPECT_EQ(_tankSpawner->IsPlayerTwoNeedRespawn(), false);
	{
		const ObjRectangle rect{.x = _tankSize * 2, .y = 0, .w = _tankSize, .h = _tankSize};
		std::string name = "Player";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		auto enemy = std::make_shared<PlayerTwo>(
				rect, _green, _tankHealth, _windowBuffer, _windowSize, UP, _tankSpeed, &_allObjects, _events, name,
				fraction, std::move(inputProvider), _bulletPool, false, true, 2);
	}
	EXPECT_EQ(_tankSpawner->IsPlayerTwoNeedRespawn(), true);
}

TEST_F(TankSpawnerTest, EnemyDiedRespawnCount)
{
	const int respawnResource = _tankSpawner->GetEnemyRespawnResource();
	{
		const ObjRectangle rect{.x = _tankSize * 2, .y = 0, .w = _tankSize, .h = _tankSize};
		constexpr int gray = 0x808080;
		auto enemy = std::make_unique<Enemy>(
				rect, gray, _tankHealth, _windowBuffer, _windowSize, DOWN, _tankSpeed, &_allObjects, _events, "Enemy",
				"EnemyTeam", _bulletPool, false, 1);
	}
	EXPECT_GT(respawnResource, _tankSpawner->GetEnemyRespawnResource());
}

TEST_F(TankSpawnerTest, PlayerOneDiedRespawnCount)
{
	const int respawnResource = _tankSpawner->GetPlayerOneRespawnResource();
	{
		const ObjRectangle rect{.x = _tankSize * 2, .y = 0, .w = _tankSize, .h = _tankSize};
		std::string name = "Player";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		auto enemy = std::make_shared<PlayerOne>(
				rect, _yellow, _tankHealth, _windowBuffer, _windowSize, UP, _tankSpeed, &_allObjects, _events, name,
				fraction, std::move(inputProvider), _bulletPool, false, 1);
	}
	EXPECT_GT(respawnResource, _tankSpawner->GetPlayerOneRespawnResource());
}

TEST_F(TankSpawnerTest, PlayerTwoDiedRespawnCount)
{
	const int respawnResource = _tankSpawner->GetPlayerTwoRespawnResource();
	{
		const ObjRectangle rect{.x = _tankSize * 2, .y = 0, .w = _tankSize, .h = _tankSize};
		std::string name = "Player";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(name, _events);
		auto enemy = std::make_shared<PlayerTwo>(
				rect, _green, _tankHealth, _windowBuffer, _windowSize, UP, _tankSpeed, &_allObjects, _events, name,
				fraction, std::move(inputProvider), _bulletPool, false, true, 2);
	}
	EXPECT_GT(respawnResource, _tankSpawner->GetPlayerTwoRespawnResource());
}
