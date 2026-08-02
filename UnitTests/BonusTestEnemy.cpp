#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "components/managers/BonusEffectManager.h"
#include "entities/obstacles/FortressWall.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/Player.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include <memory>

class BonusTestEnemy : public testing::Test// NOLINT(clang-diagnostic-padded)
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
	buuid _uuid{};
	double _deltaTimeOneFrame{1.f / 60.f};
	float _tankSize{};
	float _gridSize{};
	float _tankSpeed{142};
	unsigned short _tankHealth{100u};
	GameMode _gameMode{GameMode::OnePlayer};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _gameConfig);
		_respawnManager = std::make_shared<RespawnManager>(_events);
		_tankSpawner = std::make_shared<TankSpawner>(_gameConfig, &_allObjects, _events, *_respawnManager);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, &_allObjects, _gameConfig);
		_bonusEffectManager = std::make_unique<BonusEffectManager>(_events);
		_gridSize = static_cast<float>(_gameConfig.windowSize.y) / 50.f;
		_tankSize = _gridSize * 3;// for better turns
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};


// NOTE: when the enemy picks up bonusShovel, then fortressWalls hide (destroy) brick walls around it
TEST_F(BonusTestEnemy, ShovelPickUpByEnemyThenFortressBricWallkHide)
{
	// spawn Enemy
	const ObjRectangle rectEnemy{.x = 0, .y = 0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);

	// spawn FortressWall
	auto fortressWall = std::make_shared<FortressWall>(
			ObjRectangle{.x = _tankSize + 1.f, .y = 0, .w = _gridSize, .h = _gridSize}, _events, &_allObjects,
			_uuid, _gameMode);
	_allObjects.emplace_back(fortressWall);

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, BonusType::Shovel);

	EXPECT_TRUE(fortressWall->IsBrickWall());
	EXPECT_NE(fortressWall->GetHealth(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_TRUE(fortressWall->IsBrickWall());
	EXPECT_EQ(fortressWall->GetHealth(), -1);
}

// NOTE: player pickup bonusShovel, then fortressWalls become steelWalls (BonusShovel_Pickup),
//       then enemy pickup bonusShovel, then fortressWalls should hide (destroy) steel walls around it
TEST_F(BonusTestEnemy, ShovelPickUpByEnemyThenFortressSteelWallHide)
{
	// spawn Enemy
	const ObjRectangle rectEnemy{.x = 0, .y = 0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);

	// spawn Player
	_allObjects.reserve(4);
	const ObjRectangle rectPlayer{.x = _tankSize * 2.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);
	bool isPressed{true};
	_events->EmitEvent("P1_Move_Down", isPressed);

	// spawn FortressWall
	const ObjRectangle fortressRect{.x = _tankSize * 3.f + 1.f, .y = _tankSize * 3.f, .w = _tankSize, .h = _tankSize};
	auto fortressWall = std::make_shared<FortressWall>(fortressRect, _events, &_allObjects, _uuid, _gameMode);
	_allObjects.emplace_back(fortressWall);

	if (fortressWall)
	{
		// spawn bonuses
		const ObjRectangle enemyBonusRect = {.x = 0.f, .y = _tankSize + 3.f, .w = _tankSize, .h = _tankSize};

		_bonusSpawner->SpawnBonus(enemyBonusRect, BonusType::Shovel);
		const ObjRectangle playerBonusRect = {.x = _tankSize * 2.f,
											  .y = _tankSize * 2.f + _tankSize + 1.f,
											  .w = _tankSize,
											  .h = _tankSize};
		_bonusSpawner->SpawnBonus(playerBonusRect, BonusType::Shovel);

		EXPECT_TRUE(fortressWall->IsBrickWall());

		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		_allObjects.pop_back();//NOTE: to avoid second pickup by player same bonus

		EXPECT_TRUE(fortressWall->IsSteelWall());
		EXPECT_NE(fortressWall->GetHealth(), 0);

		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_TRUE(fortressWall->IsBrickWall());
		EXPECT_EQ(fortressWall->GetHealth(), -1);

		return;
	}

	EXPECT_TRUE(false);
}
