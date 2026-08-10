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

class CoopBotTest : public testing::Test// NOLINT(clang-diagnostic-padded)
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
		TestUtils::WireSpawnQueue(_events, &_allObjects);
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
		_events->RemoveListener("AddToSpawnQueue", "TestSpawnQueue");
	}
};

// Check that Coop bot does not change direction if it can't see the bonus
TEST_F(CoopBotTest, CoopNoChangeDirIfBonusOutsideLineOfSight)
{
	// Spawn coop
	const ObjRectangle coopBotRect{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<CoopBot> coopBot =
			TestUtils::CreateTank<CoopBot>(
					coopBotRect, _tankHealth, _uuid, "CoopBot1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn a bonus diagonally, out of the bot's direct line of sight
	_bonusSpawner->SpawnRandomBonus({.x = _tankSize * 2.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize});

	const Direction startDirCoop = coopBot->GetDirection();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const Direction endDirCoop = coopBot->GetDirection();

	EXPECT_EQ(startDirCoop, endDirCoop);
}

// Check that coop bot shoots when seeing an enemy
TEST_F(CoopBotTest, CoopShootToEnemy)
{
	// Spawn coop
	const ObjRectangle coopBotRect{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<CoopBot> coopBot =
			TestUtils::CreateTank<CoopBot>(
					coopBotRect, _tankHealth, _uuid, "CoopBot1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Enemy in line of sight
	const ObjRectangle rectEnemy{.x = 0.f, .y = _tankSize * 3.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	const size_t sizeBefore = _allObjects.size();
	EXPECT_EQ(sizeBefore, 2u);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	const size_t sizeAfter = _allObjects.size();
	EXPECT_LT(sizeBefore, sizeAfter);// Bullet should be spawned
	EXPECT_EQ(sizeAfter, 4u);
}

// Check that CoopBot does not shoot at PlayerTeam members
TEST_F(CoopBotTest, CoopNoShootToCoop)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<CoopBot> coopBot =
			TestUtils::CreateTank<CoopBot>(
					coopBotRect, _tankHealth, _uuid, "CoopBot1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn another coopBot in line of sight first one
	const ObjRectangle coopBotRect2 = {.x = 0.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<CoopBot> coopBot2 =
			TestUtils::CreateTank<CoopBot>(
					coopBotRect2, _tankHealth, _uuid, "CoopBot2", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot2);

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

// Check that CoopBot does not shoot at PlayerTeam members
TEST_F(CoopBotTest, CoopNoShootToPlayer1)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<CoopBot> coopBot =
			TestUtils::CreateTank<CoopBot>(
					coopBotRect, _tankHealth, _uuid, "CoopBot1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Player in line of sight Coop
	const ObjRectangle rectPlayer{.x = 0.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

// Check that Coop can shoot at Brick wall
TEST_F(CoopBotTest, CoopShootToBrick)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<CoopBot> coopBot =
			TestUtils::CreateTank<CoopBot>(
					coopBotRect, _tankHealth, _uuid, "CoopBot1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn BrickWall
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<BrickWall>(rect, _events, _uuid, _gameMode));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_LT(sizeBefore, _allObjects.size());
}

// Check that Coop not shoots at Brick wall if too close
TEST_F(CoopBotTest, CoopTooCloseToShootTheBrick)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<CoopBot> coopBot =
			TestUtils::CreateTank<CoopBot>(
					coopBotRect, _tankHealth, _uuid, "CoopBot1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn BrickWall
	const ObjRectangle rect{.x = 0.f, .y = _tankSize + 18.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<BrickWall>(rect, _events, _uuid, _gameMode));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

// Check that Coop can shoot at Steel wall if he can destroy it
TEST_F(CoopBotTest, CoopShootToSteel)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<CoopBot> coopBot =
			TestUtils::CreateTank<CoopBot>(
					coopBotRect, _tankHealth, _uuid, "CoopBot1", "PlayerTeam", &_allObjects, _events, 3u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn SteelWall
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<SteelWall>(rect, _events, _uuid, _gameMode));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_LT(sizeBefore, _allObjects.size());
}

// Check that Coop can shoot at Steel wall if low tier
TEST_F(CoopBotTest, CoopNoShootToSteelIfTierTooLow)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<CoopBot> coopBot =
			TestUtils::CreateTank<CoopBot>(
					coopBotRect, _tankHealth, _uuid, "CoopBot1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn SteelWall
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<SteelWall>(rect, _events, _uuid, _gameMode));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

// Check that Coop can shoot at Eagle
TEST_F(CoopBotTest, CoopNoShootToEagle)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<CoopBot> coopBot =
			TestUtils::CreateTank<CoopBot>(
					coopBotRect, _tankHealth, _uuid, "CoopBot1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Eagle
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<EagleTile>(rect, _events, _uuid, _gameMode));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

// Check that Coop can shoot at Fortress wall
TEST_F(CoopBotTest, CoopNoShootToFortress)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<CoopBot> coopBot =
			TestUtils::CreateTank<CoopBot>(
					coopBotRect, _tankHealth, _uuid, "CoopBot1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn FortressWall
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<FortressWall>(rect, _events, &_allObjects, _uuid, _gameMode));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

// Check that Coop can shoot at Water
TEST_F(CoopBotTest, CoopNoShootToWater)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<CoopBot> coopBot =
			TestUtils::CreateTank<CoopBot>(
					coopBotRect, _tankHealth, _uuid, "CoopBot1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Water
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<WaterTile>(rect, _events, _uuid, _gameMode));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

// Check that Coop can shoot at Bush
TEST_F(CoopBotTest, CoopNoShootToBush)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<CoopBot> coopBot =
			TestUtils::CreateTank<CoopBot>(
					coopBotRect, _tankHealth, _uuid, "CoopBot1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Bush
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<BushTile>(rect, _events, _uuid, _gameMode));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(sizeBefore, _allObjects.size());
}

// Check that Coop can shoot at Ice
TEST_F(CoopBotTest, CoopNoShootToIce)
{
	// Spawn Coop
	const ObjRectangle coopBotRect{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<CoopBot> coopBot =
			TestUtils::CreateTank<CoopBot>(
					coopBotRect, _tankHealth, _uuid, "CoopBot1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(coopBot);

	// Spawn Ice
	const ObjRectangle rect{.x = 0.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize};
	_allObjects.emplace_back(std::make_shared<IceTile>(rect, _events, _uuid, _gameMode));

	const size_t sizeBefore = _allObjects.size();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(sizeBefore, _allObjects.size());
}
