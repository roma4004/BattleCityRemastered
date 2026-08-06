#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/SpawnEvents.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "components/managers/BonusEffectManager.h"
#include "entities/bonuses/Bonus.h"
#include "entities/obstacles/FortressWall.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/Player.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include <memory>

class BonusTest : public testing::Test// NOLINT(clang-diagnostic-padded)
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
	unsigned short _tankHealth{100u};
	unsigned short _bulletHealth{1u};
	float _tankSize{};
	float _gridSize{};
	float _tankSpeed{142};
	double _deltaTimeOneFrame{1.f / 60.f};
	BulletCalibre _calibre{.speed = 300.f, .damage = 1u, .damageRadius = 12.0, .tier = 1u, .size{.x = 6.f, .y = 5.f}};
	buuid _uuid{};
	GameMode _gameMode{GameMode::OnePlayer};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		TestUtils::WireSpawnQueue(_events, &_allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _gameConfig);
		_respawnManager = std::make_shared<RespawnManager>(_events);
		_tankSpawner = std::make_shared<TankSpawner>(_gameConfig, &_allObjects, _events);
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

// Check that tank can pick up a random bonus
TEST_F(BonusTest, BonusPickUp)
{
	// spawn Player
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);

	_allObjects.emplace_back(player);
	_bonusSpawner->SpawnRandomBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize});

	constexpr bool isPressed{true};
	_events->EmitEvent("Move_Down", Key(std::string{"P1"}), isPressed);

	if (const auto bonus = dynamic_cast<Bonus*>(_allObjects.back().get()))
	{
		EXPECT_TRUE(bonus->GetIsAlive());

		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_FALSE(bonus->GetIsAlive());
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

// Check that tank can pick up a random bonus
TEST_F(BonusTest, BonusNotPickUp)
{
	// spawn Player
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	_bonusSpawner->SpawnRandomBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize});

	constexpr bool isPressed{true};
	_events->EmitEvent("Move_Up", Key(std::string{"P1"}), isPressed);

	if (const auto bonus = dynamic_cast<Bonus*>(_allObjects.back().get()))
	{
		EXPECT_TRUE(bonus->GetIsAlive());

		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_TRUE(bonus->GetIsAlive());
	}
	else
	{
		EXPECT_TRUE(false);
	}
}

// Check that player can pick up Timer bonus and freeze enemy
TEST_F(BonusTest, TimerPickUpEnemyCantMove)
{
	// spawn Player
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);
	constexpr bool isPressed{true};
	_events->EmitEvent("Move_Down", Key(std::string{"P1"}), isPressed);

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, BonusType::Timer);

	// spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 2, .y = _tankSize * 2, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const FPoint enemyPos = enemyBot->GetPos();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(enemyPos, enemyBot->GetPos());
}

// Check that player not pick up Timer bonus and enemies still move
TEST_F(BonusTest, TimerNotPickUpEnemyCanMove)
{
	// spawn Player
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);
	constexpr bool isPressed{true};
	_events->EmitEvent("Move_Up", Key(std::string{"P1"}), isPressed);

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, BonusType::Timer);

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

//Check that player can pick up Helmet bonus and enemies can't damage player
TEST_F(BonusTest, HelmetPickUpAndBulletCantDamageTank)
{
	// spawn Player
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);
	constexpr bool isPressed{true};
	_events->EmitEvent("Move_Down", Key(std::string{"P1"}), isPressed);
	const int playerHealth = player->GetHealth();

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, BonusType::Helmet);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	// spawn Bullet
	const ObjRectangle rectBullet{.x = _tankSize + 1.f, .y = 0.f, .w = 6.f, .h = 5.f};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "EnemyTeam", &_allObjects,
					_events, _calibre, Direction::LEFT, _gameMode, _gameConfig, "Enemy1");
	_allObjects.emplace_back(bullet);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(playerHealth, player->GetHealth());
}

//Check that player not pick up Helmet bonus and enemies can damage player
TEST_F(BonusTest, HelmetNotPickUpBulletCanDamageTank)
{
	// spawn Player
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);
	constexpr bool isPressed{true};
	_events->EmitEvent("Move_Up", Key(std::string{"P1"}), isPressed);
	const int playerHealth = player->GetHealth();

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, BonusType::Helmet);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	// spawn Bullet
	const ObjRectangle rectBullet{.x = _tankSize + 1.f, .y = 0.f, .w = 6.f, .h = 5.f};
	std::shared_ptr<Bullet> bullet =
			TestUtils::CreateBullet(
					rectBullet, _bulletHealth, _uuid, "Bullet1", "EnemyTeam", &_allObjects,
					_events, _calibre, Direction::LEFT, _gameMode, _gameConfig, "Enemy1");
	_allObjects.emplace_back(bullet);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_NE(playerHealth, player->GetHealth());
}

//Check that player pick up Grenade bonus and enemies got zero health
TEST_F(BonusTest, GrenadePickUpEnemyHealthZero)
{
	// spawn Player
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);
	constexpr bool isPressed{true};
	_events->EmitEvent("Move_Down", Key(std::string{"P1"}), isPressed);

	// spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 2, .y = _tankSize * 2, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	EXPECT_EQ(enemyBot->GetHealth(), 100);

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, BonusType::Grenade);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(enemyBot->GetHealth(), 0);
}

//Check that not player pick up Grenade bonus and enemies remain full health
TEST_F(BonusTest, GrenadeNotPickUpEnemyHealthFull)
{
	// spawn Player
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);
	constexpr bool isPressed{true};
	_events->EmitEvent("Move_Up", Key(std::string{"P1"}), isPressed);

	// spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 2, .y = _tankSize * 2, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	EXPECT_EQ(enemyBot->GetHealth(), 100);

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, BonusType::Grenade);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(enemyBot->GetHealth(), 100);
}

//Check that player pick up Tank bonus and got his extra life
TEST_F(BonusTest, TankPickUpExtraLife)
{
	unsigned short respawnActual{3u};
	_events->AddListener(
			"RespawnCountChangedTo", "BonusTest",
			[&respawnActual](const RespawnCountChangedToEvent& event)
			{
				respawnActual = event.respawnCount;
			});

	// spawn Player
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);
	constexpr bool isPressed{true};
	_events->EmitEvent("Move_Down", Key(std::string{"P1"}), isPressed);

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, BonusType::Tank);

	const unsigned short playerSpawnCount = respawnActual;

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_LT(playerSpawnCount, respawnActual);

	_events->RemoveListener("RespawnCountChangedTo", "GameStateManagerTest");
}

//Check that player not pick up Tank bonus and his life count remains the same
TEST_F(BonusTest, TankNotPickUpTierTheSame)
{
	unsigned short respawnActual{3u};
	_events->AddListener(
			"RespawnCountChangedTo", "BonusTest",
			[&respawnActual](const RespawnCountChangedToEvent& event)
			{
				respawnActual = event.respawnCount;
			});

	// spawn Player
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);
	constexpr bool isPressed{true};
	_events->EmitEvent("Move_Up", Key(std::string{"P1"}), isPressed);

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, BonusType::Tank);

	const unsigned short playerSpawnCount = respawnActual;

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(playerSpawnCount, respawnActual);

	_events->RemoveListener("RespawnCountChangedTo", "GameStateManagerTest");
}

//Check that player pick up Star bonus and his tier increased
TEST_F(BonusTest, StarPickUpTierIncrease)
{
	// spawn Player
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);
	constexpr bool isPressed{true};
	_events->EmitEvent("Move_Down", Key(std::string{"P1"}), isPressed);

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, BonusType::Star);

	EXPECT_EQ(player->GetTier(), 1u);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(player->GetTier(), 2u);
}

//Check that player not pick up Star bonus and his tier remains the same
TEST_F(BonusTest, StarNotPickUpTierTheSame)
{
	// spawn Player
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);
	constexpr bool isPressed{true};
	_events->EmitEvent("Move_Up", Key(std::string{"P1"}), isPressed);

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, BonusType::Star);

	EXPECT_EQ(player->GetTier(), 1u);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(player->GetTier(), 1u);
}

// NOTE: when player pick up shovel bonus fortressWalls become steelWalls for a while then return to regular brickWalls
//Check that player pick up Shovel bonus and Fortress wall turns into Steel wall
TEST_F(BonusTest, ShovelPickUpByPlayerThenFortressWallTurnIntoSteelWall)
{
	// spawn Player
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);
	constexpr bool isPressed{true};
	_events->EmitEvent("Move_Down", Key(std::string{"P1"}), isPressed);

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, BonusType::Shovel);

	const auto fortressWall =
			std::make_shared<FortressWall>(ObjRectangle{.x = _tankSize + 1.f, .y = 0, .w = _gridSize, .h = _gridSize},
										   _events, &_allObjects, _uuid, _gameMode);

	EXPECT_TRUE(fortressWall->IsBrickWall());

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_TRUE(fortressWall->IsSteelWall());
}

//TODO: add new tests, that count bricks and check that player can pickup bonus and rebuild fortress and skip if space spawn not available
//Check that player not pick up Shovel bonus and his Fortress wall remain the same
TEST_F(BonusTest, ShovelNotPickUpByFortressWallTheSame)
{
	// spawn Player
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);
	constexpr bool isPressed{true};
	_events->EmitEvent("Move_Up", Key(std::string{"P1"}), isPressed);

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, BonusType::Shovel);

	const auto fortressWall =
			std::make_shared<FortressWall>(ObjRectangle{.x = _tankSize + 1.f, .y = 0, .w = _gridSize, .h = _gridSize},
										   _events, &_allObjects, _uuid, _gameMode);

	EXPECT_TRUE(fortressWall->IsBrickWall());

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_TRUE(fortressWall->IsBrickWall());
}
