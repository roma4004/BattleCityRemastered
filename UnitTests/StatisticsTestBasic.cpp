#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/managers/BonusManager.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/InputEvents.h"
#include "components/events/TimingEvents.h"
#include "components/GameStatistics.h"
#include "entities/bonuses/Bonus.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Bot.h"
#include "entities/pawns/Player.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/InputChannel.h"
#include "gtest/gtest.h"
#include "enums/Faction.h"
#include <memory>
#include <thread>

class StatisticsTest : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<GameStatistics> _statistics{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<BonusManager> _bonusManager{nullptr};
	std::vector<EventSubscription> _instantSpawnAnimationSubs{};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.0 / 60.0};
	BulletCalibre _calibre{.speed = 300.0, .damage = 1u, .damageRadius = 12.0, .tier = 1u, .size{.x = 6.0, .y = 5.0}};
	double _tankSize{};
	double _tankSpeed{142.0};
	Uuid _uuid{};
	unsigned short _tankHealth{1u};
	unsigned short _bulletHealth{1u};
	GameMode _gameMode{GameMode::OnePlayer};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, _allObjects, _gameConfig);
		_bonusManager = std::make_shared<BonusManager>(_events, _gameConfig);
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
		_statistics = std::make_shared<GameStatistics>(_events);
		const double gridSize = _gameConfig.gridOffset;
		_tankSize = gridSize * 3.0;// for better turns

		_allObjects.reserve(5);
	}

	void TearDown() override {}
};

TEST_F(StatisticsTest, PlayerOneHitByEnemy)
{
	// Spawn Player1
	const ObjRectangle rectPlayer{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	// spawn Bullet
	const ObjRectangle rectBullet{.x = _tankSize / 2.0, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::EnemyTeam, _allObjects,
					_events, _calibre, Direction::UP, _gameMode, _gameConfig, Author::Enemy1);
	_allObjects.emplace_back(bullet);

	EXPECT_EQ(_statistics->GetData().playerOneHitByEnemyTeam, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().playerOneHitByEnemyTeam, 1u);
}

TEST_F(StatisticsTest, PlayerOneHitByFriend)
{
	// Spawn Player1
	const ObjRectangle rectPlayer{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	// spawn Bullet
	const ObjRectangle rectBullet{.x = _tankSize / 2.0,
								  .y = _tankSize + 1.0,
								  .w = _calibre.size.x,
								  .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::PlayerTeam, _allObjects,
					_events, _calibre, Direction::UP, _gameMode, _gameConfig, Author::Player2);
	_allObjects.emplace_back(bullet);

	EXPECT_EQ(_statistics->GetData().playerOneHitFriendlyFire, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().playerOneHitFriendlyFire, 1u);
}

TEST_F(StatisticsTest, PlayerTwoHitByEnemy)
{
	// Spawn Player2
	const ObjRectangle rectPlayer2{.x = _tankSize + 1.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player2 =
			TestUtils::CreateTank<Player>(
					rectPlayer2, _tankHealth, _uuid, Author::Player2, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player2);

	// spawn Bullet
	const ObjRectangle rectBullet{.x = _tankSize + _tankSize / 2.0,
								  .y = _tankSize,
								  .w = _calibre.size.x,
								  .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::EnemyTeam, _allObjects,
					_events, _calibre, Direction::UP, _gameMode, _gameConfig, Author::Enemy1);
	_allObjects.emplace_back(bullet);

	EXPECT_EQ(_statistics->GetData().playerTwoHitByEnemyTeam, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().playerTwoHitByEnemyTeam, 1u);
}

TEST_F(StatisticsTest, PlayerTwoHitByFriend)
{
	// Spawn Player2
	const ObjRectangle rectPlayer2{.x = _tankSize + 1.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player2 =
			TestUtils::CreateTank<Player>(
					rectPlayer2, _tankHealth, _uuid, Author::Player2, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player2);

	// spawn Bullet
	const ObjRectangle rectBullet{.x = _tankSize + _tankSize / 2.0,
								  .y = _tankSize,
								  .w = _calibre.size.x,
								  .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::PlayerTeam, _allObjects,
					_events, _calibre, Direction::UP, _gameMode, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);

	EXPECT_EQ(_statistics->GetData().playerTwoHitFriendlyFire, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().playerTwoHitFriendlyFire, 1u);
}

TEST_F(StatisticsTest, PlayerOneDiedByFriend)
{
	// Spawn Player1
	const ObjRectangle rectPlayer{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.0, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::PlayerTeam, _allObjects,
					_events, _calibre, Direction::UP, _gameMode, _gameConfig, Author::Player2);
	_allObjects.emplace_back(bullet);

	EXPECT_EQ(_statistics->GetData().playerOneDiedByFriendlyFire, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().playerOneDiedByFriendlyFire, 1u);
}

TEST_F(StatisticsTest, PlayerTwoDiedByEnemy)
{
	// Spawn Player2
	const ObjRectangle rectPlayer2{.x = _tankSize + 1.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player2 =
			TestUtils::CreateTank<Player>(
					rectPlayer2, _tankHealth, _uuid, Author::Player2, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player2);

	// spawn Bullet
	const ObjRectangle rectBullet{.x = _tankSize + _tankSize / 2.0,
								  .y = _tankSize,
								  .w = _calibre.size.x,
								  .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::EnemyTeam, _allObjects,
					_events, _calibre, Direction::UP, _gameMode, _gameConfig, Author::Enemy1);
	_allObjects.emplace_back(bullet);

	EXPECT_EQ(_statistics->GetData().playerDiedByEnemyTeam, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().playerDiedByEnemyTeam, 1u);
}

TEST_F(StatisticsTest, PlayerOneDiedByEnemy)
{
	// Spawn Player1
	const ObjRectangle rectPlayer{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	// spawn Bullet
	const ObjRectangle rectBullet{.x = _calibre.size.x, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::EnemyTeam, _allObjects,
					_events, _calibre, Direction::UP, _gameMode, _gameConfig, Author::Enemy1);
	_allObjects.emplace_back(bullet);

	EXPECT_EQ(_statistics->GetData().playerDiedByEnemyTeam, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().playerDiedByEnemyTeam, 1u);
}

TEST_F(StatisticsTest, PlayerTwoDiedByFriend)
{
	// Spawn Player2
	const ObjRectangle rectPlayer2{.x = _tankSize + 1.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player2 =
			TestUtils::CreateTank<Player>(
					rectPlayer2, _tankHealth, _uuid, Author::Player2, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player2);

	// spawn Bullet
	const ObjRectangle rectBullet{.x = _tankSize + _tankSize / 2.0,
								  .y = _tankSize,
								  .w = _calibre.size.x,
								  .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::PlayerTeam, _allObjects,
					_events, _calibre, Direction::UP, _gameMode, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);

	EXPECT_EQ(_statistics->GetData().playerTwoDiedByFriendlyFire, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().playerTwoDiedByFriendlyFire, 1u);
}

TEST_F(StatisticsTest, EnemyHitByFriend)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 2.0 + 2.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Bot> enemyBot =
			TestUtils::CreateTank<Bot>(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// spawn Bullet
	const ObjRectangle rectBullet{.x = _tankSize * 2.0 + 2.0 + _tankSize / 2.0,
								  .y = _tankSize,
								  .w = _calibre.size.x,
								  .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::EnemyTeam, _allObjects,
					_events, _calibre, Direction::UP, _gameMode, _gameConfig, Author::Enemy2);
	_allObjects.emplace_back(bullet);

	EXPECT_EQ(_statistics->GetData().enemyHitByFriendlyFire, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().enemyHitByFriendlyFire, 1u);
}

TEST_F(StatisticsTest, EnemyHitByPlayerOne)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 2.0 + 2.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Bot> enemyBot =
			TestUtils::CreateTank<Bot>(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// spawn Bullet
	const ObjRectangle rectBullet{.x = _tankSize * 2.0 + 2.0 + _tankSize / 2.0,
								  .y = _tankSize,
								  .w = _calibre.size.x,
								  .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::PlayerTeam, _allObjects,
					_events, _calibre, Direction::UP, _gameMode, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);

	EXPECT_EQ(_statistics->GetData().enemyHitByPlayerOne, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().enemyHitByPlayerOne, 1u);
}

TEST_F(StatisticsTest, EnemyHitByPlayerTwo)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 2.0 + 2.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Bot> enemyBot =
			TestUtils::CreateTank<Bot>(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// spawn Bullet
	const ObjRectangle rectBullet{.x = _tankSize * 2.0 + 2.0 + _tankSize / 2.0,
								  .y = _tankSize + 1,
								  .w = _calibre.size.x,
								  .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::PlayerTeam, _allObjects,
					_events, _calibre, Direction::UP, _gameMode, _gameConfig, Author::Player2);
	_allObjects.emplace_back(bullet);

	EXPECT_EQ(_statistics->GetData().enemyHitByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().enemyHitByPlayerTwo, 1u);
}

TEST_F(StatisticsTest, EnemyDiedByFriend)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 2.0 + 2.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Bot> enemyBot =
			TestUtils::CreateTank<Bot>(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// spawn Bullet
	const ObjRectangle rectBullet{.x = _tankSize * 2.0 + 2.0 + _tankSize / 2.0,
								  .y = _tankSize,
								  .w = _calibre.size.x,
								  .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::EnemyTeam, _allObjects,
					_events, _calibre, Direction::UP, _gameMode, _gameConfig, Author::Enemy2);
	_allObjects.emplace_back(bullet);

	EXPECT_EQ(_statistics->GetData().enemyDiedByFriendlyFire, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().enemyDiedByFriendlyFire, 1u);
}

TEST_F(StatisticsTest, EnemyDiedByPlayerOne)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 2.0 + 2.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Bot> enemyBot =
			TestUtils::CreateTank<Bot>(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// spawn Bullet
	const ObjRectangle rectBullet{.x = _tankSize * 2.0 + 2.0 + _tankSize / 2.0,
								  .y = _tankSize + 1,
								  .w = _calibre.size.x,
								  .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::PlayerTeam, _allObjects,
					_events, _calibre, Direction::UP, _gameMode, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);

	EXPECT_EQ(_statistics->GetData().enemyDiedByPlayerOne, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().enemyDiedByPlayerOne, 1u);
}

TEST_F(StatisticsTest, EnemyDiedByPlayerTwo)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 2.0 + 2.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Bot> enemyBot =
			TestUtils::CreateTank<Bot>(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// spawn Bullet
	const ObjRectangle rectBullet{.x = _tankSize * 2.0 + 2.0 + _tankSize / 2.0,
								  .y = _tankSize,
								  .w = _calibre.size.x,
								  .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::PlayerTeam, _allObjects,
					_events, _calibre, Direction::UP, _gameMode, _gameConfig, Author::Player2);
	_allObjects.emplace_back(bullet);

	EXPECT_EQ(_statistics->GetData().enemyDiedByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().enemyDiedByPlayerTwo, 1u);
}

TEST_F(StatisticsTest, BulletHitByPlayerTwo)
{
	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.0, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::PlayerTeam, _allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);

	// spawn Bullet2
	const ObjRectangle rectBullet2{.x = 0.0,
								   .y = _tankSize + _calibre.size.y + 1.0,
								   .w = _calibre.size.x,
								   .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet2 =
			TestUtils::CreateBullet(
					rectBullet2, _bulletHealth, _uuid, "Bullet2", Faction::PlayerTeam, _allObjects,
					_events, _calibre, Direction::UP, _gameMode, _gameConfig, Author::Player2);
	_allObjects.emplace_back(bullet2);

	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerOne, 1u);
	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerTwo, 1u);
}

TEST_F(StatisticsTest, BrickWallDiedByEnemy)
{
	// spawn BrickWall
	ObjRectangle brickWallRect{.x = 0.0,
							   .y = _tankSize + _calibre.size.y + 1,
							   .w = _calibre.size.x,
							   .h = _calibre.size.y};

	_allObjects.emplace_back(std::make_shared<BrickWall>(brickWallRect, _events, _uuid, _gameMode));

	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.0, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::EnemyTeam, _allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, Author::Enemy1);
	_allObjects.emplace_back(bullet);

	EXPECT_EQ(_statistics->GetData().brickWallDiedByEnemyTeam, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().brickWallDiedByEnemyTeam, 1u);
}

TEST_F(StatisticsTest, BrickWallDiedByPlayerOne)
{
	// spawn BrickWall
	ObjRectangle brickWallRect{.x = 0.0,
							   .y = _tankSize + _calibre.size.y + 1,
							   .w = _calibre.size.x,
							   .h = _calibre.size.y};
	_allObjects.emplace_back(std::make_shared<BrickWall>(brickWallRect, _events, _uuid, _gameMode));

	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.0, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::PlayerTeam, _allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);

	EXPECT_EQ(_statistics->GetData().brickWallDiedByPlayerOne, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().brickWallDiedByPlayerOne, 1u);
}

TEST_F(StatisticsTest, BrickDiedByPlayerTwo)
{
	// spawn BrickWall
	ObjRectangle brickRect{.x = 0.0, .y = _tankSize + _calibre.size.y + 1, .w = _calibre.size.x, .h = _calibre.size.y};
	_allObjects.emplace_back(std::make_shared<BrickWall>(brickRect, _events, _uuid, _gameMode));

	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.0, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::PlayerTeam, _allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, Author::Player2);
	_allObjects.emplace_back(bullet);

	EXPECT_EQ(_statistics->GetData().brickWallDiedByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().brickWallDiedByPlayerTwo, 1u);
}

TEST_F(StatisticsTest, SteelWallDiedByEnemy)
{
	// spawn SteelWall
	ObjRectangle brickWallRect{.x = 0.0,
							   .y = _tankSize + _calibre.size.y + 1,
							   .w = _calibre.size.x,
							   .h = _calibre.size.y};
	_allObjects.emplace_back(std::make_shared<SteelWall>(brickWallRect, _events, _uuid, _gameMode));

	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.0, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	_calibre.tier = 3u;
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::EnemyTeam, _allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, Author::Enemy1);
	_allObjects.emplace_back(bullet);

	EXPECT_EQ(_statistics->GetData().steelWallDiedByEnemyTeam, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().steelWallDiedByEnemyTeam, 1u);
}

TEST_F(StatisticsTest, SteelWallDiedByPlayerOne)
{
	// spawn SteelWall
	ObjRectangle brickWallRect{.x = 0.0,
							   .y = _tankSize + _calibre.size.y + 1,
							   .w = _calibre.size.x,
							   .h = _calibre.size.y};
	_allObjects.emplace_back(std::make_shared<SteelWall>(brickWallRect, _events, _uuid, _gameMode));

	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.0, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	_calibre.tier = 3u;
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::PlayerTeam, _allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);

	EXPECT_EQ(_statistics->GetData().steelWallDiedByPlayerOne, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().steelWallDiedByPlayerOne, 1u);
}

TEST_F(StatisticsTest, SteelDiedByPlayerTwo)
{
	// spawn SteelWall
	ObjRectangle brickRect{.x = 0.0, .y = _tankSize + _calibre.size.y + 1, .w = _calibre.size.x, .h = _calibre.size.y};
	_allObjects.emplace_back(std::make_shared<SteelWall>(brickRect, _events, _uuid, _gameMode));

	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.0, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	_calibre.tier = 3u;
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::PlayerTeam, _allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, Author::Player2);
	_allObjects.emplace_back(bullet);

	EXPECT_EQ(_statistics->GetData().steelWallDiedByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().steelWallDiedByPlayerTwo, 1u);
}

TEST_F(StatisticsTest, BulletHitBulletByEnemyAndByEnemy)
{
	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.0, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::EnemyTeam, _allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, Author::Enemy1);
	_allObjects.emplace_back(bullet);

	// spawn Bullet2
	const ObjRectangle rectBullet2{.x = 0.0,
								   .y = _tankSize + _calibre.size.y + 1.0,
								   .w = _calibre.size.x,
								   .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet2 =
			TestUtils::CreateBullet(
					rectBullet2, _bulletHealth, _uuid, "Bullet2", Faction::EnemyTeam, _allObjects,
					_events, _calibre, Direction::UP, _gameMode, _gameConfig, Author::Enemy2);
	_allObjects.emplace_back(bullet2);

	EXPECT_EQ(_statistics->GetData().bulletHitByEnemy, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bulletHitByEnemy, 2u);
}

TEST_F(StatisticsTest, BulletHitBulletPlayerOneAndByPlayerTwo)
{
	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.0, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::PlayerTeam, _allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);

	// spawn Bullet2
	const ObjRectangle rectBullet2{.x = 0.0,
								   .y = _tankSize + _calibre.size.y + 1.0,
								   .w = _calibre.size.x,
								   .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet2 =
			TestUtils::CreateBullet(
					rectBullet2, _bulletHealth, _uuid, "Bullet2", Faction::PlayerTeam, _allObjects,
					_events, _calibre, Direction::UP, _gameMode, _gameConfig, Author::Player2);
	_allObjects.emplace_back(bullet2);

	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerOne, 1u);
	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerTwo, 1u);
}

TEST_F(StatisticsTest, BulletHitBulletByEnemyAndByPlayerOne)
{
	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.0, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::PlayerTeam, _allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, Author::Player1);
	_allObjects.emplace_back(bullet);

	// spawn Bullet2
	const ObjRectangle rectBullet2{.x = 0.0,
								   .y = _tankSize + _calibre.size.y + 1.0,
								   .w = _calibre.size.x,
								   .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet2 =
			TestUtils::CreateBullet(
					rectBullet2, _bulletHealth, _uuid, "Bullet2", Faction::EnemyTeam, _allObjects,
					_events, _calibre, Direction::UP, _gameMode, _gameConfig, Author::Enemy1);
	_allObjects.emplace_back(bullet2);

	EXPECT_EQ(_statistics->GetData().bulletHitByEnemy, 0u);
	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerOne, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bulletHitByEnemy, 1u);
	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerOne, 1u);
}

TEST_F(StatisticsTest, BulletHitBulletByEnemyAndByPlayerTwo)
{
	// spawn Bullet
	const ObjRectangle rectBullet{.x = 0.0, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::PlayerTeam, _allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, Author::Player2);
	_allObjects.emplace_back(bullet);

	// spawn Bullet2
	const ObjRectangle rectBullet2{.x = 0.0,
								   .y = _tankSize + _calibre.size.y + 1.0,
								   .w = _calibre.size.x,
								   .h = _calibre.size.y};
	std::shared_ptr<Bullet> bullet2 =
			TestUtils::CreateBullet(
					rectBullet2, _bulletHealth, _uuid, "Bullet2", Faction::EnemyTeam, _allObjects,
					_events, _calibre, Direction::UP, _gameMode, _gameConfig, Author::Enemy1);
	_allObjects.emplace_back(bullet2);

	EXPECT_EQ(_statistics->GetData().bulletHitByEnemy, 0u);
	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bulletHitByEnemy, 1u);
	EXPECT_EQ(_statistics->GetData().bulletHitByPlayerTwo, 1u);
}

// Check that tank can pick up a random bonus with statistic count
TEST_F(StatisticsTest, BonusPickUpByEnemyCount)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Bot> enemyBot =
			TestUtils::CreateTank<Bot>(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize});
	_bonusSpawner->SpawnRandomBonus({.x = _tankSize + 1.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 1u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 0u);
}

// Check that tank can pick up a random bonus with statistic count
TEST_F(StatisticsTest, BonusNotPickUpByEnemyNotCount)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Bot> enemyBot =
			TestUtils::CreateTank<Bot>(
					rectEnemy, _tankHealth, _uuid, Author::Enemy1, Faction::EnemyTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize * 2 + 1.0, .w = _tankSize, .h = _tankSize});
	_bonusSpawner->SpawnRandomBonus({.x = _tankSize * 2 + 1.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 0u);
}

// Check that tank can pick up a random bonus with statistic count
TEST_F(StatisticsTest, BonusPickUpByPlayerOneCount)
{
	// Spawn Player1
	const ObjRectangle rectPlayer{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize});
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 1u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 0u);
}

// Check that tank can pick up a random bonus with statistic count
TEST_F(StatisticsTest, BonusNotPickUpByPlayerOneNotCount)
{
	// Spawn Player1
	const ObjRectangle rectPlayer{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, Author::Player1, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = isPressed});

	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 0u);
}

// Check that tank can pick up a random bonus with statistic count
TEST_F(StatisticsTest, BonusPickUpByPlayerTwoCount)
{
	// Spawn Player2
	const ObjRectangle rectPlayer2{.x = _tankSize + 1.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player2 =
			TestUtils::CreateTank<Player>(
					rectPlayer2, _tankHealth, _uuid, Author::Player2, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player2);
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP2), MoveDownEvent{.isPressed = isPressed});

	_bonusSpawner->SpawnRandomBonus({.x = _tankSize + 1.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 1u);
}

// Check that tank can pick up a random bonus with statistic count
TEST_F(StatisticsTest, BonusNotPickUpByPlayerTwoNotCount)
{
	// Spawn Player2
	const ObjRectangle rectPlayer2{.x = _tankSize + 1.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player2 =
			TestUtils::CreateTank<Player>(
					rectPlayer2, _tankHealth, _uuid, Author::Player2, Faction::PlayerTeam, _allObjects, _events, 1u,
					_tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player2);
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP2), MoveUpEvent{.isPressed = isPressed});

	_bonusSpawner->SpawnRandomBonus({.x = _tankSize + 1.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 0u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bonusPickupByEnemyTeam, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerTwo, 0u);
}

TEST_F(StatisticsTest, BonusExpiredCountedWithNoAuthor)
{
	using namespace std::chrono_literals;

	//NOTE: how long a bonus lives is the spawner's, so the test shortens it instead of building one by hand
	_gameConfig.bonusLifeTimeCooldown = 1ms;
	_bonusSpawner->SpawnBonus({.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize}, BonusType::Helmet);
	ASSERT_EQ(_allObjects.size(), 1u);

	EXPECT_EQ(_statistics->GetData().bonusExpired, 0u);

	std::this_thread::sleep_for(2ms);
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bonusExpired, 1u);
	EXPECT_FALSE(_allObjects.back()->GetIsAlive());

	//NOTE: the timer is one-shot - a second tick must not keep counting the same bonus
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(_statistics->GetData().bonusExpired, 1u);
}

TEST_F(StatisticsTest, BonusShotIsCountedAndPickupIsNot)
{
	using namespace std::chrono_literals;

	const ObjRectangle rectBonus{.x = 0.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	auto shot = std::make_shared<Bonus>(rectBonus, _events, _uuid, _gameMode, BonusType::Helmet, false);
	_allObjects.emplace_back(shot);

	shot->TakeDamage(1u, Author::Player1);

	EXPECT_FALSE(shot->GetIsAlive());
	EXPECT_EQ(_statistics->GetData().bonusDestroyedByPlayerOne, 1u);
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 0u);

	auto taken = std::make_shared<Bonus>(rectBonus, _events, _uuid, _gameMode, BonusType::Helmet, false);
	_allObjects.emplace_back(taken);

	taken->PickUpBonus(Author::Player1);

	EXPECT_FALSE(taken->GetIsAlive());
	EXPECT_EQ(_statistics->GetData().bonusPickupByPlayerOne, 1u);
	EXPECT_EQ(_statistics->GetData().bonusDestroyedByPlayerOne, 1u);
}
