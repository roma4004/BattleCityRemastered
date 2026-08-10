#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "components/managers/BonusEffectManager.h"
#include "entities/bonuses/Bonus.h"
#include "entities/obstacles/FortressWall.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/BulletResetProperty.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/Player.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include <memory>

class BonusTestDestroy : public testing::Test// NOLINT(clang-diagnostic-padded)
{
	using buuid = boost::uuids::uuid;

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::unique_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<RespawnManager> _respawnManager{nullptr};
	std::shared_ptr<BonusEffectManager> _bonusEffectManager{nullptr};
	GameConfig _gameConfig{"", true};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.f / 60.f};
	BulletCalibre _calibre{.speed = 300.f, .damage = 1u, .damageRadius = 12.0, .tier = 1u, .size{.x = 6.f, .y = 5.f}};
	float _tankSize{};
	float _gridSize{};
	float _tankSpeed{142};
	unsigned short _tankHealth{100u};
	unsigned short _bulletHealth{1u};
	buuid _uuid{};
	GameMode _gameMode{GameMode::OnePlayer};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		TestUtils::WireSpawnQueue(_events, &_allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _gameConfig);
		_respawnManager = std::make_shared<RespawnManager>(_events);
		_tankSpawner = std::make_shared<TankSpawner>(_gameConfig, &_allObjects, _events, *_respawnManager);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, &_allObjects, _gameConfig);
		_bonusEffectManager = std::make_unique<BonusEffectManager>(_events);
		_gridSize = static_cast<float>(_gameConfig.windowSize.y) / 50.f;
		_tankSize = _gridSize * 3.f;// for better turns

		_allObjects.reserve(4);
	}

	void TearDown() override
	{
		_events->RemoveListener("AddToSpawnQueue", "TestSpawnQueue");
	}
};

// Check that bullet can destroy a random bonus
TEST_F(BonusTestDestroy, BonusDestroy)
{
	// spawn Bullet
	constexpr ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = 6.f, .h = 5.f};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "EnemyTeam", &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Enemy1");
	_allObjects.emplace_back(bullet);

	_bonusSpawner->SpawnRandomBonus({.x = 0.f, .y = 7.f, .w = _tankSize, .h = _tankSize});

	if (const auto bonus = dynamic_cast<Bonus*>(_allObjects.back().get()))
	{
		EXPECT_TRUE(bonus->GetIsAlive());

		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_FALSE(bonus->GetIsAlive());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that bullet not destroy a random bonus
TEST_F(BonusTestDestroy, BonusNotDestroy)
{
	// spawn Bullet
	constexpr ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = 6.f, .h = 5.f};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "EnemyTeam", &_allObjects,
					_events, _calibre, Direction::RIGHT, _gameMode, _gameConfig, "Enemy1");
	_allObjects.emplace_back(bullet);

	_bonusSpawner->SpawnRandomBonus({.x = 0.f, .y = 7.f, .w = _tankSize, .h = _tankSize});

	if (const auto bonus = dynamic_cast<Bonus*>(_allObjects.back().get()))
	{
		EXPECT_TRUE(bonus->GetIsAlive());

		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_TRUE(bonus->GetIsAlive());

		return;
	}

	EXPECT_TRUE(false);
}

//Check that player can destroy timer bonus and enemies still move
TEST_F(BonusTestDestroy, TimerDestroyByPlayerAndEnemyStillMove)
{
	// spawn Bullet
	constexpr ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = 6.f, .h = 5.f};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "PlayerTeam", &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = 7.f, .w = _tankSize, .h = _tankSize}, BonusType::Timer);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	// spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 2, .y = _tankSize * 2, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const FPoint enemyPos = enemyBot->GetPos();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_NE(enemyPos, enemyBot->GetPos());
}

//Check that player can destroy helmet bonus and enemies still can damage player
TEST_F(BonusTestDestroy, HelmetDestroyAndBulletStillCanDamageTank)
{
	// spawn Player
	const ObjRectangle rectPlayer{.x = _tankSize + 1.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player2", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	// spawn Bullet
	constexpr ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = 6.f, .h = 5.f};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "PlayerTeam", &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = 7.f, .w = _tankSize, .h = _tankSize}, BonusType::Helmet);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	// spawn Bullet2
	const ObjRectangle rectBullet2{.x = _tankSize * 2 + 1.f, .y = 7.f, .w = 6.f, .h = 5.f};
	std::shared_ptr<Bullet> bullet2 =
			TestUtils::CreateBullet(
					rectBullet2, _bulletHealth, _uuid, "Bullet2", "EnemyTeam", &_allObjects,
					_events, _calibre, Direction::LEFT, _gameMode, _gameConfig, "Enemy1");
	_allObjects.emplace_back(bullet2);

	const int playerHealth = player->GetHealth();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_NE(playerHealth, player->GetHealth());
}


//Check that player can destroy Grenade bonus and enemies still full health
TEST_F(BonusTestDestroy, GrenadeDestroyEnemyHealthFull)
{
	// spawn Bullet
	constexpr ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = 6.f, .h = 5.f};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "PlayerTeam", &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = 7.f, .w = _tankSize, .h = _tankSize}, BonusType::Grenade);

	// spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 2, .y = _tankSize * 2, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	EXPECT_EQ(enemyBot->GetHealth(), 100);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(enemyBot->GetHealth(), 100);
}

//Check that player destroys Tank bonus and his life counts remain the same
TEST_F(BonusTestDestroy, TankDestroyNoExtraLife)
{
	unsigned short respawnActual{3u};
	_events->AddListener(
			"RespawnCountChangedTo", "BonusTest",
			[&respawnActual](const std::string& /*objectName*/, const unsigned short respawnCount)
			{
				respawnActual = respawnCount;
			});

	// spawn Bullet
	constexpr ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = 6.f, .h = 5.f};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "PlayerTeam", &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = 7.f, .w = _tankSize, .h = _tankSize}, BonusType::Tank);

	const unsigned short playerSpawnCount = respawnActual;

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(playerSpawnCount, respawnActual);

	_events->RemoveListener("RespawnCountChangedTo", "GameStateManagerTest");
}

//Check that player destroys Star bonus and his tier counts remain the same
TEST_F(BonusTestDestroy, StarDestroyTierRemainTheSame)
{
	// spawn Player
	const ObjRectangle rectPlayer{.x = 0.f, .y = _tankSize * 3.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player2", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	// spawn Bullet
	constexpr ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = 6.f, .h = 5.f};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "PlayerTeam", &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = 7.f, .w = _tankSize, .h = _tankSize}, BonusType::Star);

	EXPECT_EQ(player->GetTier(), 1u);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(player->GetTier(), 1u);
}

//Check that player destroys Shovel bonus and fortress brick remain the same
TEST_F(BonusTestDestroy, ShovelNotPickUpByPlayerThenfortressWallRemainTheSame)
{
	// spawn Bullet
	constexpr ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = 6.f, .h = 5.f};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "PlayerTeam", &_allObjects,
					_events, _calibre, Direction::DOWN, _gameMode, _gameConfig, "Player1");
	_allObjects.emplace_back(bullet);

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = 7.f, .w = _tankSize, .h = _tankSize}, BonusType::Shovel);
	const auto fortressWall =
			std::make_shared<FortressWall>(ObjRectangle{.x = _tankSize + 1.f, .y = 0, .w = _gridSize, .h = _gridSize},
										   _events, &_allObjects, _uuid, _gameMode);

	EXPECT_TRUE(fortressWall->IsBrickWall());

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_TRUE(fortressWall->IsBrickWall());
}
