#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/TimingEvents.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "components/managers/BonusManager.h"
#include "entities/bonuses/Bonus.h"
#include "components/ObstacleSpawner.h"
#include "components/managers/FortressManager.h"
#include "entities/obstacles/FortressWalls.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/BulletResetProperty.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/Player.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/ObstacleType.h"
#include "gtest/gtest.h"
#include "enums/Faction.h"
#include <memory>

class BonusTestDestroy : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::unique_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::vector<EventSubscription> _instantSpawnAnimationSubs{};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<RespawnManager> _respawnManager{nullptr};
	std::shared_ptr<BonusManager> _bonusManager{nullptr};
	std::unique_ptr<FortressManager> _fortressManager{nullptr};
	std::unique_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::shared_ptr<BaseObj> _fortressWall{nullptr};
	EventSubscription _fortressWallSub{};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.0 / 60.0};
	BulletCalibre _calibre{.speed = 300.0, .damage = 1u, .damageRadius = 12.0, .tier = 1u, .size{.x = 6.0, .y = 5.0}};
	double _tankSize{};
	double _gridSize{};
	double _tankSpeed{142};
	unsigned short _tankHealth{100u};
	unsigned short _bulletHealth{1u};
	Uuid _uuid{};
	GameMode _gameMode{GameMode::OnePlayer};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, &_allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _gameConfig);
		TestUtils::ApplyGameMode(_events, &_allObjects, _gameConfig, _gameConfig.gameMode, _respawnManager,
								 _tankSpawner);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, &_allObjects, _gameConfig);
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
		_bonusManager = std::make_unique<BonusManager>(_events, _gameConfig);
		_fortressManager = std::make_unique<FortressManager>(_events, &_allObjects);
		_obstacleSpawner = std::make_unique<ObstacleSpawner>(_events, _gameConfig);
		_fortressWallSub = TestUtils::TrackFortressWall(_events, &_fortressWall);
		_gridSize = _gameConfig.gridOffset;
		_tankSize = _gridSize * 3.0;// for better turns

		_allObjects.reserve(4);
	}

	void TearDown() override
	{
	}
};

// Check that bullet can destroy a random bonus
TEST_F(BonusTestDestroy, BonusDestroy)
{
	// spawn Bullet
	constexpr ObjRectangle rectBullet{.x = 0.0, .y = 0.0, .w = 6.0, .h = 5.0};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::EnemyTeam, &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Enemy1");
	_allObjects.emplace_back(bullet);

	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = 7.0, .w = _tankSize, .h = _tankSize});

	if (const auto bonus = dynamic_cast<Bonus*>(_allObjects.back().get()))
	{
		EXPECT_TRUE(bonus->GetIsAlive());

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_FALSE(bonus->GetIsAlive());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that bullet not destroy a random bonus
TEST_F(BonusTestDestroy, BonusNotDestroy)
{
	// spawn Bullet
	constexpr ObjRectangle rectBullet{.x = 0.0, .y = 0.0, .w = 6.0, .h = 5.0};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::EnemyTeam, &_allObjects,
					_events, _calibre, Direction::RIGHT, _gameMode, _gameConfig, "Enemy1");
	_allObjects.emplace_back(bullet);

	_bonusSpawner->SpawnRandomBonus({.x = 0.0, .y = 7.0, .w = _tankSize, .h = _tankSize});

	if (const auto bonus = dynamic_cast<Bonus*>(_allObjects.back().get()))
	{
		EXPECT_TRUE(bonus->GetIsAlive());

		_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

		EXPECT_TRUE(bonus->GetIsAlive());

		return;
	}

	EXPECT_TRUE(false);
}

//Check that player can destroy timer bonus and enemies still move
TEST_F(BonusTestDestroy, TimerDestroyByPlayerAndEnemyStillMove)
{
	// spawn Bullet
	constexpr ObjRectangle rectBullet{.x = 0.0, .y = 0.0, .w = 6.0, .h = 5.0};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::PlayerTeam, &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = 7.0, .w = _tankSize, .h = _tankSize}, BonusType::Timer);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	// spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 2, .y = _tankSize * 2, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", Faction::EnemyTeam, &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const FPoint enemyPos = enemyBot->GetPos();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_NE(enemyPos, enemyBot->GetPos());
}

//Check that player can destroy helmet bonus and enemies still can damage player
TEST_F(BonusTestDestroy, HelmetDestroyAndBulletStillCanDamageTank)
{
	// spawn Player
	const ObjRectangle rectPlayer{.x = _tankSize + 1.0, .y = 0.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player2", Faction::PlayerTeam, &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	// spawn Bullet
	constexpr ObjRectangle rectBullet{.x = 0.0, .y = 0.0, .w = 6.0, .h = 5.0};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::PlayerTeam, &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = 7.0, .w = _tankSize, .h = _tankSize}, BonusType::Helmet);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	// spawn Bullet2
	const ObjRectangle rectBullet2{.x = _tankSize * 2 + 1.0, .y = 7.0, .w = 6.0, .h = 5.0};
	std::shared_ptr<Bullet> bullet2 =
			TestUtils::CreateBullet(
					rectBullet2, _bulletHealth, _uuid, "Bullet2", Faction::EnemyTeam, &_allObjects,
					_events, _calibre, Direction::LEFT, _gameMode, _gameConfig, "Enemy1");
	_allObjects.emplace_back(bullet2);

	const int playerHealth = player->GetHealth();

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_NE(playerHealth, player->GetHealth());
}


//Check that player can destroy Grenade bonus and enemies still full health
TEST_F(BonusTestDestroy, GrenadeDestroyEnemyHealthFull)
{
	// spawn Bullet
	constexpr ObjRectangle rectBullet{.x = 0.0, .y = 0.0, .w = 6.0, .h = 5.0};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::PlayerTeam, &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = 7.0, .w = _tankSize, .h = _tankSize}, BonusType::Grenade);

	// spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 2, .y = _tankSize * 2, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", Faction::EnemyTeam, &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	EXPECT_EQ(enemyBot->GetHealth(), 100);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(enemyBot->GetHealth(), 100);
}

//Check that player destroys Tank bonus and his life counts remain the same
TEST_F(BonusTestDestroy, TankDestroyNoExtraLife)
{
	unsigned short respawnActual{3u};
	auto respawnSub = _events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		respawnActual = event.respawnCount;
	});

	// spawn Bullet
	constexpr ObjRectangle rectBullet{.x = 0.0, .y = 0.0, .w = 6.0, .h = 5.0};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::PlayerTeam, &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = 7.0, .w = _tankSize, .h = _tankSize}, BonusType::Tank);

	const unsigned short playerSpawnCount = respawnActual;

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(playerSpawnCount, respawnActual);

}

//Check that player destroys Star bonus and his tier counts remain the same
TEST_F(BonusTestDestroy, StarDestroyTierRemainTheSame)
{
	// spawn Player
	const ObjRectangle rectPlayer{.x = 0.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player2", Faction::PlayerTeam, &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	// spawn Bullet
	constexpr ObjRectangle rectBullet{.x = 0.0, .y = 0.0, .w = 6.0, .h = 5.0};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::PlayerTeam, &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = 7.0, .w = _tankSize, .h = _tankSize}, BonusType::Star);

	EXPECT_EQ(player->GetTier(), 1u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(player->GetTier(), 1u);
}

//Check that player destroys Shovel bonus and fortress brick remain the same
TEST_F(BonusTestDestroy, ShovelNotPickUpByPlayerThenfortressWallRemainTheSame)
{
	// spawn Bullet
	constexpr ObjRectangle rectBullet{.x = 0.0, .y = 0.0, .w = 6.0, .h = 5.0};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", Faction::PlayerTeam, &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = 7.0, .w = _tankSize, .h = _tankSize}, BonusType::Shovel);
	const ObjRectangle fortressRect{.x = _tankSize + 1.0, .y = 0, .w = _gridSize, .h = _gridSize};
	_events->EmitEvent(SpawnObstacleEvent{.rect = fortressRect, .type = ObstacleType::Fortress});

	EXPECT_NE(dynamic_cast<FortressBrickWall*>(_fortressWall.get()), nullptr);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_NE(dynamic_cast<FortressBrickWall*>(_fortressWall.get()), nullptr);
}
