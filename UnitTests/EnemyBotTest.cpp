#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/TankSpawner.h"
#include "components/managers/DelayedSpawnManager.h"
#include "components/managers/RespawnManager.h"
#include "components/managers/GameStateManager.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/BushTile.h"
#include "entities/obstacles/EagleTile.h"
#include "entities/obstacles/FortressWall.h"
#include "entities/obstacles/IceTile.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/obstacles/WaterTile.h"
#include "entities/pawns/CoopBot.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/Player.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include <memory>

class EnemyBotTest : public testing::Test// NOLINT(clang-diagnostic-padded)
{
	using buuid = boost::uuids::uuid;

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::unique_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<GameStateManager> _stateManager{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<RespawnManager> _respawnManager{nullptr};
	std::shared_ptr<DelayedSpawnManager> _spawnDelayManager{nullptr};
	GameConfig _gameConfig{"", true};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.f / 60.f};
	buuid _uuid{};
	float _tankSize{};
	float _tankSpeed{142};
	float _gridSize{};
	unsigned short _tankHealth{100u};
	GameMode _gameMode{GameMode::OnePlayer};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _gameConfig);
		_stateManager = std::make_shared<GameStateManager>(_events);
		_respawnManager = std::make_shared<RespawnManager>(_events);
		_tankSpawner = std::make_shared<TankSpawner>(_gameConfig, &_allObjects, _events, *_respawnManager);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, &_allObjects, _gameConfig);
		_spawnDelayManager = std::make_shared<DelayedSpawnManager>(_events);
		_gridSize = static_cast<float>(_gameConfig.windowSize.y) / 50.f;
		_tankSize = _gridSize * 3;// for better turns

		_allObjects.reserve(4u);
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

// Check that bot shoots when seeing an opponent
TEST_F(EnemyBotTest, EnemyShootToCoop)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.f, .y = _tankSize * 3.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<CoopBot> coopBot =
			TestUtils::CreateTank<CoopBot>(
					coopBotRect, _tankHealth, _uuid, "CoopBot1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Enemy in line of sight
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 2u);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const size_t sizeAfter = _allObjects.size();
	EXPECT_LT(sizeBefore, sizeAfter);// Bullet should be spawned
	EXPECT_EQ(sizeAfter, 4u);
}

// Check that bot shoots when seeing a Player1
TEST_F(EnemyBotTest, EnemyShootToPlayer1)
{
	// Spawn Enemy	
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);

	_allObjects.emplace_back(enemyBot);

	// Spawn Player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.f, .y = _tankSize * 3.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 2u);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const size_t sizeAfter = _allObjects.size();
	EXPECT_LT(sizeBefore, sizeAfter);// Bullet should be spawned
	EXPECT_EQ(sizeAfter, 3u);
}

// Check that bot shoots when seeing a Player2
TEST_F(EnemyBotTest, EnemyShootToPlayer2)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn Player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.f, .y = _tankSize * 3.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player2", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 2u);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const size_t sizeAfter = _allObjects.size();
	EXPECT_LT(sizeBefore, sizeAfter);// Bullet should be spawned
	EXPECT_EQ(sizeAfter, 3u);
}

// Check that bot shoots when seeing a Player1
TEST_F(EnemyBotTest, EnemyNoShootToPlayer1IfTooClose)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn Player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.f, .y = _tankSize * 2.f + 7.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 2u);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const size_t sizeAfter = _allObjects.size();
	EXPECT_LT(sizeBefore, sizeAfter);// Bullet should be spawned
	EXPECT_EQ(sizeAfter, 3u);
}

// Check that bot shoots when seeing a Player2
TEST_F(EnemyBotTest, EnemyNoShootToPlayer2IfTooClose)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn Player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.f, .y = _tankSize * 2.f + 7.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player2", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 2u);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const size_t sizeAfter = _allObjects.size();
	EXPECT_LT(sizeBefore, sizeAfter);// Bullet should be spawned
	EXPECT_EQ(sizeAfter, 3u);
}

// check that enemy don't shoot the allied tanks
TEST_F(EnemyBotTest, EnemyNoShootToAllied)
{
	// Spawn first Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn second Enemy in line of sight of the first
	const ObjRectangle rectEnemy2 = {.x = 0.f, .y = _tankSize * 3.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<BaseObj> enemyBot2BaseObj =
			TestUtils::CreateTank<Enemy>(
					rectEnemy2, _tankHealth, _uuid, "Enemy2", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot2BaseObj);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 2u);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

// check that enemy doesn't shoot the allied tanks even if too close to them
TEST_F(EnemyBotTest, EnemyNoShootToAlliedIfTooClose)
{
	// Spawn first enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn second Enemy in line of sight of the first
	const ObjRectangle rectEnemy2 = {.x = 0.f, .y = _tankSize * 2.f + 6.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<BaseObj> enemyBot2BaseObj =
			TestUtils::CreateTank<Enemy>(
					rectEnemy2, _tankHealth, _uuid, "Enemy2", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot2BaseObj);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 2u);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

// Check that Enemy can shoot at Brick wall
TEST_F(EnemyBotTest, EnemyShootToBrick)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<BrickWall>(rect, _events, _uuid, _gameMode));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_LT(sizeBefore, _allObjects.size());
}

// Check that Enemy not shoots at Brick wall if too close
TEST_F(EnemyBotTest, EnemyTooCloseToShootTheBrick)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const ObjRectangle rect{.x = 0.f, .y = _tankSize + 3.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<BrickWall>(rect, _events, _uuid, _gameMode));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

// Check that enemy can shoot at Steel wall if he can destroy it
TEST_F(EnemyBotTest, EnemyShootToSteel)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 3u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn SteelWall
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<SteelWall>(rect, _events, _uuid, _gameMode));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_LT(sizeBefore, _allObjects.size());
}

// Check that Enemy can shoot at Steel wall if low tier
TEST_F(EnemyBotTest, EnemyNoShootToSteelIfTierTooLow)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn SteelWall
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<SteelWall>(rect, _events, _uuid, _gameMode));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

// Check that Enemy can shoot at Eagle
TEST_F(EnemyBotTest, EnemyShootToEagle)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn Eagle
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<EagleTile>(rect, _events, _uuid, _gameMode));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_LT(sizeBefore, _allObjects.size());
}

// Check that Enemy can shoot at Fortress wall
TEST_F(EnemyBotTest, EnemyShootToFortress)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn FortressWall
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<FortressWall>(rect, _events, &_allObjects, _uuid, _gameMode));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_LT(sizeBefore, _allObjects.size());
}

// Check that Enemy can shoot at Water
TEST_F(EnemyBotTest, EnemyShootToWater)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn Water
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<WaterTile>(rect, _events, _uuid, _gameMode));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

// Check that Enemy can shoot at Bush
TEST_F(EnemyBotTest, EnemyShootToBush)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn Bush
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<BushTile>(rect, _events, _uuid, _gameMode));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

// Check that Enemy can shoot at Ice
TEST_F(EnemyBotTest, EnemyShootToIce)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn Ice
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<IceTile>(rect, _events, _uuid, _gameMode));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

// Check that Enemy can shoot at Player that been behind water
TEST_F(EnemyBotTest, EnemyShootToPlayerBehindWater)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn Water
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f + 1.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<WaterTile>(rect, _events, _uuid, _gameMode));

	// Spawn player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.f, .y = _tankSize * 3.f + 2.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 3u);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const size_t sizeAfter = _allObjects.size();
	EXPECT_LT(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, 4u);
}

// Check that Enemy can shoot at Player that been in the water (in case of BonusShip was pickup)
TEST_F(EnemyBotTest, EnemyShootToPlayerInTheWater)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn Water
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f + 1.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<WaterTile>(rect, _events, _uuid, _gameMode));

	// Spawn player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.f, .y = _tankSize * 2.f + 1.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 3u);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const size_t sizeAfter = _allObjects.size();
	EXPECT_LT(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, 4u);
}

// Check that Enemy can shoot at Player that been behind Ice
TEST_F(EnemyBotTest, EnemyShootToPlayerBehindIce)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn Ice
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f + 1.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<IceTile>(rect, _events, _uuid, _gameMode));

	// Spawn player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.f, .y = _tankSize * 3.f + 2.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 3u);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const size_t sizeAfter = _allObjects.size();
	EXPECT_LT(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, 4u);
}

// Check that Enemy can shoot at Player that been in the Ice
TEST_F(EnemyBotTest, EnemyShootToPlayerInTheIce)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn Ice
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f + 1.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<IceTile>(rect, _events, _uuid, _gameMode));

	// Spawn player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.f, .y = _tankSize * 2.f + 1.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 3u);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const size_t sizeAfter = _allObjects.size();
	EXPECT_LT(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, 4u);
}

// Check that Enemy can shoot at Player that been behind BrickWall
TEST_F(EnemyBotTest, EnemyNoShootToPlayerBehindBrickWall)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::RIGHT, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn BrickWall
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f + 1.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<BrickWall>(rect, _events, _uuid, _gameMode));

	// Spawn player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.f, .y = _tankSize * 3.f + 2.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 3u);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const size_t sizeAfter = _allObjects.size();
	EXPECT_EQ(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, 3u);
}

// Check that Enemy can shoot at Player that been behind SteelWall
TEST_F(EnemyBotTest, EnemyNoShootToPlayerBehindSteelWall)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::RIGHT, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn SteelWall
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f + 1.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<SteelWall>(rect, _events, _uuid, _gameMode));

	// Spawn player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.f, .y = _tankSize * 3.f + 2.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 3u);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const size_t sizeAfter = _allObjects.size();
	EXPECT_EQ(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, 3u);
}

// Check that Enemy can shoot at Player that been behind FortressWall
TEST_F(EnemyBotTest, EnemyNoShootToPlayerBehindFortressWall)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::RIGHT, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn FortressWall
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f + 1.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<FortressWall>(rect, _events, &_allObjects, _uuid, _gameMode));

	// Spawn player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.f, .y = _tankSize * 3.f + 2.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 3u);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const size_t sizeAfter = _allObjects.size();
	EXPECT_EQ(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, 3u);
}

// Check that Enemy can shoot at Player that been behind Bush
TEST_F(EnemyBotTest, EnemyNoShootToPlayerBehindBush)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn Bush
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f + 1.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<BushTile>(rect, _events, _uuid, _gameMode));

	// Spawn player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.f, .y = _tankSize * 3.f + 2.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 3u);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const size_t sizeAfter = _allObjects.size();
	EXPECT_EQ(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, 3u);
}

// Check that Enemy can shoot at Player that been in the Bush
TEST_F(EnemyBotTest, EnemyNoShootToPlayerInTheBush)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn Bush
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f + 1.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<BushTile>(rect, _events, _uuid, _gameMode));

	// Spawn player aligned enemy in line of sight
	const ObjRectangle rectPlayer{.x = 0.f, .y = _tankSize * 2.f + 1.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 3u);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const size_t sizeAfter = _allObjects.size();
	EXPECT_EQ(sizeBefore, sizeAfter);
	EXPECT_EQ(sizeAfter, 3u);
}
