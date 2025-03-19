#include "../headers/application/Window.h"
#include "../headers/components/BulletPool.h"
#include "../headers/components/EventSystem.h"
#include "../headers/components/GameStatistics.h"
#include "../headers/enums/Direction.h"
#include "../headers/enums/GameMode.h"
#include "../headers/input/InputProviderForPlayerOne.h"
#include "../headers/input/InputProviderForPlayerTwo.h"
#include "../headers/obstacles/BrickWall.h"
#include "../headers/obstacles/SteelWall.h"
#include "../headers/pawns/Bullet.h"
#include "../headers/pawns/Enemy.h"
#include "../headers/pawns/PlayerOne.h"
#include "../headers/pawns/PlayerTwo.h"

#include "gtest/gtest.h"

#include <memory>

class StatisticsTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<GameStatistics> _statistics{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<Window> _window{nullptr};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	int _tankHealth{1};
	int _bulletHealth{1};
	int _bulletColor{0xffffff};
	int _yellow{0xffffff};
	int _green{0x408000};
	int _gray{0x808080};
	int _bulletDamage{1};
	float _tankSize{0.f};
	float _tankSpeed{142.f};
	float _bulletSpeed{300.f};
	float _deltaTimeOneFrame{1.f / 60.f};//TODO: Implement this in other tests
	float _bulletWidth{6.f};
	float _bulletHeight{5.f};
	double _bulletDamageRadius{12.0};
	std::string _name{"Player"};
	std::string _fraction{"PlayerTeam"};
	std::string _name2{"Player"};
	std::string _fraction2{"PlayerTeam"};
	GameMode _gameMode{OnePlayer};

	void SetUp() override
	{
		_window = std::make_shared<Window>(UPoint{.x = 800, .y = 600}, std::shared_ptr<int[]>());
		_events = std::make_shared<EventSystem>();
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _window, _gameMode);
		_statistics = std::make_shared<GameStatistics>(_events);
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(_name, _events);
		std::unique_ptr<IInputProvider> inputProvider2 = std::make_unique<InputProviderForPlayerTwo>(_name2, _events);
		const float gridSize = static_cast<float>(_window->size.y) / 50.f;
		_tankSize = gridSize * 3.f;// for better turns
		const ObjRectangle playerRect{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
		const ObjRectangle player2Rect{.x = _tankSize + 1.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
		const ObjRectangle enemy1Rect{.x = _tankSize * 2.f + 2.f, .y = 0.f, .w = _tankSize, .h = _tankSize};

		_allObjects.reserve(5);
		_allObjects.emplace_back(
				std::make_shared<PlayerOne>(
						playerRect, _yellow, _tankHealth, _window, UP, _tankSpeed, &_allObjects, _events, _name,
						_fraction, std::move(inputProvider), _bulletPool, _gameMode, 1));
		_allObjects.emplace_back(
				std::make_shared<PlayerTwo>(
						player2Rect, _green, _tankHealth, _window, UP, _tankSpeed, &_allObjects, _events, _name2,
						_fraction2, std::move(inputProvider2), _bulletPool, _gameMode, 2));
		_allObjects.emplace_back(
				std::make_shared<Enemy>(
						enemy1Rect, _gray, _tankHealth, _window, DOWN, _tankSpeed, &_allObjects, _events, "Enemy",
						"EnemyTeam", _bulletPool, _gameMode, 1));
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

TEST_F(StatisticsTest, PlayerOneHitByEnemy)
{
	ObjRectangle bulletRect{.x = _tankSize / 2.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, UP,
					_bulletSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam", _gameMode, 0));

	EXPECT_EQ(_statistics->GetPlayerOneHitByEnemyTeam(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerOneHitByEnemyTeam(), 1);
}

TEST_F(StatisticsTest, PlayerOneHitByFriend)
{
	const ObjRectangle bulletRect{.x = _tankSize / 2.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, UP,
					_bulletSpeed, &_allObjects, _events, "Player2", "PlayerTeam", _gameMode, 0));
	EXPECT_EQ(_statistics->GetPlayerOneHitFriendlyFire(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerOneHitFriendlyFire(), 1);
}

TEST_F(StatisticsTest, PlayerTwoHitByEnemy)
{
	const ObjRectangle bulletRect{.x = _tankSize + _tankSize / 2.f, .y = _tankSize, .w = _bulletWidth,
	                              .h = _bulletHeight};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, UP,
					_bulletSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam", _gameMode, 0));

	EXPECT_EQ(_statistics->GetPlayerTwoHitByEnemyTeam(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerTwoHitByEnemyTeam(), 1);
}

TEST_F(StatisticsTest, PlayerTwoHitByFriend)
{
	ObjRectangle bulletRect{.x = _tankSize + _tankSize / 2.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, UP,
					_bulletSpeed, &_allObjects, _events, "Player1", "PlayerTeam", _gameMode, 0));

	EXPECT_EQ(_statistics->GetPlayerTwoHitFriendlyFire(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerTwoHitFriendlyFire(), 1);
}

TEST_F(StatisticsTest, PlayerOneDiedByFriend)
{
	ObjRectangle bulletRect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, UP,
					_bulletSpeed, &_allObjects, _events, "Player2", "PlayerTeam", _gameMode, 0));

	EXPECT_EQ(_statistics->GetPlayerOneDiedByFriendlyFire(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerOneDiedByFriendlyFire(), 1);
}

TEST_F(StatisticsTest, PlayerTwoDiedByEnemy)
{
	ObjRectangle bulletRect{.x = _tankSize + _tankSize / 2.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, UP,
					_bulletSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam", _gameMode, 0));

	EXPECT_EQ(_statistics->GetPlayerDiedByEnemyTeam(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerDiedByEnemyTeam(), 1);
}

TEST_F(StatisticsTest, PlayerOneDiedByEnemy)
{
	ObjRectangle bulletRect{.x = _tankSize / 2.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, UP,
					_bulletSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam", _gameMode, 0));

	EXPECT_EQ(_statistics->GetPlayerDiedByEnemyTeam(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerDiedByEnemyTeam(), 1);
}

TEST_F(StatisticsTest, PlayerTwoDiedByFriend)
{
	ObjRectangle bulletRect{.x = _tankSize + _tankSize / 2.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, UP,
					_bulletSpeed, &_allObjects, _events, "Player1", "PlayerTeam", _gameMode, 0));

	EXPECT_EQ(_statistics->GetPlayerTwoDiedByFriendlyFire(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerTwoDiedByFriendlyFire(), 1);
}

TEST_F(StatisticsTest, EnemyHitByFriend)
{
	ObjRectangle bulletRect{.x = _tankSize * 2.f + 2.f + _tankSize / 2.f, .y = _tankSize, .w = _bulletWidth,
	                        .h = _bulletHeight};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, UP,
					_bulletSpeed, &_allObjects, _events, "Enemy2", "EnemyTeam", _gameMode, 0));

	EXPECT_EQ(_statistics->GetEnemyHitByFriendlyFire(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetEnemyHitByFriendlyFire(), 1);
}

TEST_F(StatisticsTest, EnemyHitByPlayerOne)
{
	ObjRectangle bulletRect{.x = _tankSize * 2.f + 2.f + _tankSize / 2.f, .y = _tankSize, .w = _bulletWidth,
	                        .h = _bulletHeight};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, UP,
					_bulletSpeed, &_allObjects, _events, "Player1", "PlayerTeam", _gameMode, 0));

	EXPECT_EQ(_statistics->GetEnemyHitByPlayerOne(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetEnemyHitByPlayerOne(), 1);
}

TEST_F(StatisticsTest, EnemyHitByPlayerTwo)
{
	ObjRectangle bulletRect{.x = _tankSize * 2.f + 2.f + _tankSize / 2.f, .y = _tankSize, .w = _bulletWidth,
	                        .h = _bulletHeight};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, UP,
					_bulletSpeed, &_allObjects, _events, "Player2", "PlayerTeam", _gameMode, 0));

	EXPECT_EQ(_statistics->GetEnemyHitByPlayerTwo(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetEnemyHitByPlayerTwo(), 1);
}

TEST_F(StatisticsTest, EnemyDiedByFriend)
{
	const ObjRectangle bulletRect{.x = _tankSize * 2.f + 2.f + _tankSize / 2.f, .y = _tankSize, .w = _bulletWidth,
	                              .h = _bulletHeight};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, UP,
					_bulletSpeed, &_allObjects, _events, "Enemy2", "EnemyTeam", _gameMode, 0));

	EXPECT_EQ(_statistics->GetEnemyDiedByFriendlyFire(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetEnemyDiedByFriendlyFire(), 1);
}

TEST_F(StatisticsTest, EnemyDiedByPlayerOne)
{
	const ObjRectangle bulletRect{.x = _tankSize * 2.f + 2.f + _tankSize / 2.f, .y = _tankSize, .w = _bulletWidth,
	                              .h = _bulletHeight};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, UP,
					_bulletSpeed, &_allObjects, _events, "Player1", "PlayerTeam", _gameMode, 0));

	EXPECT_EQ(_statistics->GetEnemyDiedByPlayerOne(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetEnemyDiedByPlayerOne(), 1);
}

TEST_F(StatisticsTest, EnemyDiedByPlayerTwo)
{
	const ObjRectangle bulletRect{.x = _tankSize * 2.f + 2.f + _tankSize / 2.f, .y = _tankSize, .w = _bulletWidth,
	                              .h = _bulletHeight};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, UP,
					_bulletSpeed, &_allObjects, _events, "Player2", "PlayerTeam", _gameMode, 0));

	EXPECT_EQ(_statistics->GetEnemyDiedByPlayerTwo(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetEnemyDiedByPlayerTwo(), 1);
}

TEST_F(StatisticsTest, BulletHitByEnemyBullet)
{
	const ObjRectangle bulletRect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	const ObjRectangle bulletRect2{.x = 0.f, .y = _tankSize + _bulletHeight + 1, .w = _bulletWidth, .h = _bulletHeight};
	constexpr int health = 1;
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, health, _window, DOWN, _bulletSpeed,
					&_allObjects, _events, "Player1", "PlayerTeam", _gameMode, 0));
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect2, _bulletDamage, _bulletDamageRadius, _bulletColor, health, _window, UP, _bulletSpeed,
					&_allObjects, _events, "Enemy1", "EnemyTeam", _gameMode, 1));

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1);
	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 1);
}

TEST_F(StatisticsTest, BulletHitByPlayerOne)
{
	_allObjects.clear();
	const ObjRectangle bulletRect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	const ObjRectangle bulletRect2{.x = 0.f, .y = _tankSize + _bulletHeight + 1.f, .w = _bulletWidth,
	                               .h = _bulletHeight};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, DOWN,
					_bulletSpeed, &_allObjects, _events, "Player2", "PlayerTeam", _gameMode, 0));
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect2, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, UP,
					_bulletSpeed, &_allObjects, _events, "Player1", "PlayerTeam", _gameMode, 1));

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 1);
}

TEST_F(StatisticsTest, BulletHitByPlayerTwo)
{
	const ObjRectangle bulletRect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	const ObjRectangle bulletRect2{.x = 0.f, .y = _tankSize + _bulletHeight + 1, .w = _bulletWidth, .h = _bulletHeight};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, DOWN,
					_bulletSpeed, &_allObjects, _events, "Player1", "PlayerTeam", _gameMode, 0));
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect2, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, UP,
					_bulletSpeed, &_allObjects, _events, "Player2", "PlayerTeam", _gameMode, 1));

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 1);
}

TEST_F(StatisticsTest, BrickWallDiedByEnemy)
{
	const ObjRectangle bulletRect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	const ObjRectangle brickWallRect{.x = 0.f, .y = _tankSize + _bulletHeight + 1, .w = _bulletWidth,
	                                 .h = _bulletHeight};
	_allObjects.emplace_back(std::make_shared<BrickWall>(brickWallRect, _window, _events, 0, _gameMode));
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, DOWN,
					_bulletSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam", _gameMode, 0));

	EXPECT_EQ(_statistics->GetBrickWallDiedByEnemyTeam(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBrickWallDiedByEnemyTeam(), 1);
}

TEST_F(StatisticsTest, BrickWallDiedByPlayerOne)
{
	const ObjRectangle bulletRect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	const ObjRectangle brickWallRect{.x = 0.f, .y = _tankSize + _bulletHeight + 1, .w = _bulletWidth,
	                                 .h = _bulletHeight};
	_allObjects.emplace_back(std::make_shared<BrickWall>(brickWallRect, _window, _events, 0, _gameMode));
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, DOWN,
					_bulletSpeed, &_allObjects, _events, "Player1", "PlayerTeam", _gameMode, 0));

	EXPECT_EQ(_statistics->GetBrickWallDiedByPlayerOne(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBrickWallDiedByPlayerOne(), 1);
}

TEST_F(StatisticsTest, BrickDiedByPlayerTwo)
{
	const ObjRectangle bulletRect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	const ObjRectangle brickRect{.x = 0.f, .y = _tankSize + _bulletHeight + 1, .w = _bulletWidth, .h = _bulletHeight};
	_allObjects.emplace_back(std::make_shared<BrickWall>(brickRect, _window, _events, 0, _gameMode));
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, DOWN,
					_bulletSpeed, &_allObjects, _events, "Player2", "PlayerTeam", _gameMode, 0));

	EXPECT_EQ(_statistics->GetBrickWallDiedByPlayerTwo(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBrickWallDiedByPlayerTwo(), 1);
}

TEST_F(StatisticsTest, SteelWallDiedByEnemy)
{
	const ObjRectangle bulletRect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	const ObjRectangle brickWallRect{.x = 0.f, .y = _tankSize + _bulletHeight + 1, .w = _bulletWidth,
	                                 .h = _bulletHeight};
	_allObjects.emplace_back(std::make_shared<SteelWall>(brickWallRect, _window, _events, 0, _gameMode));
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, DOWN,
					_bulletSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam", _gameMode, 0, 3));

	EXPECT_EQ(_statistics->GetSteelWallDiedByEnemyTeam(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetSteelWallDiedByEnemyTeam(), 1);
}

TEST_F(StatisticsTest, SteelWallDiedByPlayerOne)
{
	const ObjRectangle bulletRect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	const ObjRectangle brickWallRect{.x = 0.f, .y = _tankSize + _bulletHeight + 1, .w = _bulletWidth,
	                                 .h = _bulletHeight};
	_allObjects.emplace_back(std::make_shared<SteelWall>(brickWallRect, _window, _events, 0, _gameMode));
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, DOWN,
					_bulletSpeed, &_allObjects, _events, "Player1", "PlayerTeam", _gameMode, 0, 3));

	EXPECT_EQ(_statistics->GetSteelWallDiedByPlayerOne(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetSteelWallDiedByPlayerOne(), 1);
}

TEST_F(StatisticsTest, SteelDiedByPlayerTwo)
{
	const ObjRectangle bulletRect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	const ObjRectangle brickRect{.x = 0.f, .y = _tankSize + _bulletHeight + 1, .w = _bulletWidth, .h = _bulletHeight};
	_allObjects.emplace_back(std::make_shared<SteelWall>(brickRect, _window, _events, 0, _gameMode));
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, DOWN,
					_bulletSpeed, &_allObjects, _events, "Player2", "PlayerTeam", _gameMode, 0, 3));

	EXPECT_EQ(_statistics->GetSteelWallDiedByPlayerTwo(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetSteelWallDiedByPlayerTwo(), 1);
}

TEST_F(StatisticsTest, BulletHitBulletByEnemyAndByEnemy)
{
	const ObjRectangle bulletRect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	const ObjRectangle bulletRect2{.x = 0.f, .y = _tankSize + _bulletHeight + 1.f, .w = _bulletWidth,
	                               .h = _bulletHeight};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, DOWN,
					_bulletSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam", _gameMode, 0));
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect2, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, UP,
					_bulletSpeed, &_allObjects, _events, "Enemy2", "EnemyTeam", _gameMode, 1));

	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 2);
}

TEST_F(StatisticsTest, BulletHitBulletPlayerOneAndByPlayerTwo)
{
	const ObjRectangle bulletRect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	const ObjRectangle bulletRect2{.x = 0.f, .y = _tankSize + _bulletHeight + 1.f, .w = _bulletWidth,
	                               .h = _bulletHeight};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, DOWN,
					_bulletSpeed, &_allObjects, _events, "Player1", "PlayerTeam", _gameMode, 0));
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect2, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, UP,
					_bulletSpeed, &_allObjects, _events, "Player2", "PlayerTeam", _gameMode, 1));

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 1);
}

TEST_F(StatisticsTest, BulletHitBulletByEnemyAndByPlayerOne)
{
	const ObjRectangle bulletRect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	const ObjRectangle bulletRect2{.x = 0.f, .y = _tankSize + _bulletHeight + 1.f, .w = _bulletWidth,
	                               .h = _bulletHeight};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, DOWN,
					_bulletSpeed, &_allObjects, _events, "Player1", "PlayerTeam", _gameMode, 0));
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect2, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, UP,
					_bulletSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam", _gameMode, 1));

	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 0);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 1);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1);
}

TEST_F(StatisticsTest, BulletHitBulletByEnemyAndByPlayerTwo)
{
	const ObjRectangle bulletRect{.x = 0.f, .y = _tankSize, .w = _bulletWidth, .h = _bulletHeight};
	const ObjRectangle bulletRect2{.x = 0.f, .y = _tankSize + _bulletHeight + 1.f, .w = _bulletWidth,
	                               .h = _bulletHeight};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, DOWN,
					_bulletSpeed, &_allObjects, _events, "Player2", "PlayerTeam", _gameMode, 0));
	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					bulletRect2, _bulletDamage, _bulletDamageRadius, _bulletColor, _bulletHealth, _window, UP,
					_bulletSpeed, &_allObjects, _events, "Enemy1", "EnemyTeam", _gameMode, 1));

	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 0);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 1);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 1);
}
