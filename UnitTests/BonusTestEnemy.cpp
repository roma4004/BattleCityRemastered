#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/InputEvents.h"
#include "components/events/TimingEvents.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "components/managers/BonusManager.h"
#include "components/ObstacleSpawner.h"
#include "components/managers/FortressManager.h"
#include "entities/obstacles/FortressWalls.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/Player.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/ObstacleType.h"
#include "enums/PlayerSlot.h"
#include "gtest/gtest.h"
#include "enums/Faction.h"
#include <memory>

class BonusTestEnemy : public testing::Test// NOLINT(clang-diagnostic-padded)
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
	Uuid _uuid{};
	double _deltaTimeOneFrame{1.0 / 60.0};
	double _tankSize{};
	double _gridSize{};
	double _tankSpeed{142};
	unsigned short _tankHealth{100u};
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
	}

	void TearDown() override
	{
	}
};


// NOTE: when the enemy picks up bonusShovel, then fortressWalls hide (destroy) brick walls around it
TEST_F(BonusTestEnemy, ShovelPickUpByEnemyThenFortressBricWallkHide)
{
	// spawn Enemy
	const ObjRectangle rectEnemy{.x = 0, .y = 0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", Faction::EnemyTeam, &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);

	// register a fortress wall
	const ObjRectangle fortressRect{.x = _tankSize + 1.0, .y = 0, .w = _gridSize, .h = _gridSize};
	_events->EmitEvent(SpawnObstacleEvent{.rect = fortressRect, .type = ObstacleType::Fortress});

	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Shovel);

	EXPECT_NE(dynamic_cast<FortressBrickWall*>(_fortressWall.get()), nullptr);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(_fortressWall->GetIsAlive());
}

// NOTE: player pickup bonusShovel, then fortressWalls become steelWalls (BonusShovel_Pickup),
//       then enemy pickup bonusShovel, then fortressWalls should hide (destroy) steel walls around it
TEST_F(BonusTestEnemy, ShovelPickUpByEnemyThenFortressSteelWallHide)
{
	// spawn Enemy
	const ObjRectangle rectEnemy{.x = 0, .y = 0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", Faction::EnemyTeam, &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);

	// spawn Player
	_allObjects.reserve(4);
	const ObjRectangle rectPlayer{.x = _tankSize * 2.0, .y = _tankSize * 2.0, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", Faction::PlayerTeam, &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);
	bool isPressed{true};
	_events->EmitEvent(Key(PlayerSlot::P1), MoveDownEvent{.isPressed = isPressed});

	// register a fortress wall
	const ObjRectangle fortressRect{.x = _tankSize * 3.0 + 1.0, .y = _tankSize * 3.0, .w = _tankSize, .h = _tankSize};
	_events->EmitEvent(SpawnObstacleEvent{.rect = fortressRect, .type = ObstacleType::Fortress});

	// spawn bonuses
	const ObjRectangle enemyBonusRect = {.x = 0.0, .y = _tankSize + 3.0, .w = _tankSize, .h = _tankSize};

	_bonusSpawner->SpawnBonus(enemyBonusRect, BonusType::Shovel);
	const ObjRectangle playerBonusRect = {.x = _tankSize * 2.0,
										  .y = _tankSize * 2.0 + _tankSize + 1.0,
										  .w = _tankSize,
										  .h = _tankSize};
	_bonusSpawner->SpawnBonus(playerBonusRect, BonusType::Shovel);

	EXPECT_NE(dynamic_cast<FortressBrickWall*>(_fortressWall.get()), nullptr);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_NE(dynamic_cast<FortressSteelWall*>(_fortressWall.get()), nullptr);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_FALSE(_fortressWall->GetIsAlive());
}
