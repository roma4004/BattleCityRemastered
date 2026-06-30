#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/TankSpawner.h"
#include "components/managers/StateManager.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "components/managers/DelayedSpawnManager.h"
#include "entities/pawns/CoopBot.h"
#include "entities/pawns/Enemy.h"
#include "gtest/gtest.h"
#include <memory>

class EventSystem;
class StateManager;
class TankSpawner;
class RespawnManager;

class BotTest : public testing::Test
{
	using buuid = boost::uuids::uuid;

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::unique_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<StateManager> _stateManager{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<DelayedSpawnManager> _spawnDelayManager{nullptr};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	UPoint _windowSize{.x = 800, .y = 600};
	int _tankHealth{100};
	float _tankSize{};
	float _tankSpeed{142};
	float _gridSize{};
	double _deltaTimeOneFrame{1.f / 60.f};
	std::string _name = "CoopBot1";
	std::string _fraction = "PlayerTeam";
	buuid _uuid{};
	GameMode _gameMode{GameMode::OnePlayer};

	BulletCalibre _calibre{.speed = 300.f, .damage = 1, .damageRadius = 12.0, .tier = 1u, .size{.x = 6.f, .y = 5.f}};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _windowSize, _gameMode);
		_stateManager = std::make_shared<StateManager>(_events);
		_tankSpawner = std::make_shared<TankSpawner>(_windowSize, &_allObjects, _events);		
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, &_allObjects, _windowSize);
		_spawnDelayManager = std::make_shared<DelayedSpawnManager>(_events);
		_gridSize = static_cast<float>(_windowSize.y) / 50.f;
		_tankSize = _gridSize * 3;// for better turns

		const ObjRectangle rect{.x = 0, .y = 0, .w = _tankSize, .h = _tankSize};
		BaseObjProperty baseObjProperty{.rect = rect,
										.health = _tankHealth,
										.uuid = _uuid,
										.name = _name,
										.fraction = _fraction};
		PawnProperty pawnProperty{
				.baseObjProperty = std::move(baseObjProperty),
				.allObjects = &_allObjects,
				.events = _events,
				.tier = 1u,
				.speed = _tankSpeed,
				.windowSize = _windowSize,
				.dir = Direction::DOWN,
				.gameMode = _gameMode};
		constexpr bool enableByDefault{true};

		_allObjects.reserve(4);
		_allObjects.emplace_back(
				std::make_shared<CoopBot>(std::move(pawnProperty), _bulletPool, enableByDefault));
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

TEST_F(BotTest, ChangeDirectionIfOponentSeen)
{
	if (const auto coopBot = dynamic_cast<CoopBot*>(_allObjects.front().get()))
	{
		const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
		const float tankSize = gridSize * 3;// for better turns
		constexpr int tankHealth = 1;
		ObjRectangle rect{.x = _tankSize * 2.f, .y = 0.f, .w = tankSize, .h = tankSize};
		BaseObjProperty baseObjProperty{.rect = rect,
										.health = tankHealth,
										.uuid = _uuid,
										.name = "Enemy1",
										.fraction = "EnemyTeam"};
		PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.windowSize = _windowSize,
			.dir = Direction::DOWN,
			.gameMode = _gameMode};

		constexpr bool enableByDefault{true};
		auto enemy = std::make_shared<Enemy>(std::move(pawnProperty), std::move(_bulletPool), enableByDefault);
		_allObjects.emplace_back(enemy);

		const Direction startDirCoop = coopBot->GetDirection();
		const Direction startDirEnemy = enemy->GetDirection();

		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		const Direction endDirCoop = coopBot->GetDirection();
		const Direction endDirEnemy = enemy->GetDirection();

		EXPECT_NE(startDirCoop, endDirCoop);
		EXPECT_NE(startDirEnemy, endDirEnemy);
		EXPECT_EQ(endDirCoop, Direction::RIGHT);
		EXPECT_EQ(endDirEnemy, Direction::LEFT);

		return;
	}

	EXPECT_TRUE(false);
}

TEST_F(BotTest, ChangeDirectionIfBonusSeen)
{
	if (const auto coopBot = dynamic_cast<CoopBot*>(_allObjects.front().get()))
	{
		_bonusSpawner->SpawnRandomBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize});

		const float gridSize = static_cast<float>(_windowSize.y) / 50.f;
		const float tankSize = gridSize * 3;// for better turns
		constexpr int tankHealth = 1;
		ObjRectangle rect{.x = _tankSize * 2.f + 2.f, .y = 0.f, .w = tankSize, .h = tankSize};
		BaseObjProperty baseObjProperty{.rect = rect,
										.health = tankHealth,
										.uuid = _uuid,
										.name = "Enemy1",
										.fraction = "EnemyTeam"};
		PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.windowSize = _windowSize,
			.dir = Direction::DOWN,
			.gameMode = _gameMode};

		constexpr bool enableByDefault{true};
		auto enemy = std::make_shared<Enemy>(std::move(pawnProperty), std::move(_bulletPool), enableByDefault);
		_allObjects.emplace_back(enemy);

		const Direction startDirCoop = coopBot->GetDirection();
		const Direction startDirEnemy = enemy->GetDirection();

		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		const Direction endDirCoop = coopBot->GetDirection();
		const Direction endDirEnemy = enemy->GetDirection();

		EXPECT_NE(startDirCoop, endDirCoop);
		EXPECT_NE(startDirEnemy, endDirEnemy);
		EXPECT_EQ(endDirCoop, Direction::RIGHT);
		EXPECT_EQ(endDirEnemy, Direction::LEFT);

		return;
	}

	EXPECT_TRUE(false);
}

//TODO: implement other bot tests

// TEST_F(BotTest, ShootToOpponent)
// TEST_F(BotTest, CoopNoShootToAlly)

// TEST_F(BotTest, ShootToBrick)
// TEST_F(BotTest, ShootToSteelWithTier)
// TEST_F(BotTest, NoShootToSteel)

// TEST_F(BotTest, EnemyShootToEagleTile)
// TEST_F(BotTest, CoopNoShootToEagleTile)

// TEST_F(BotTest, EnemyShootToFortressWall)
// TEST_F(BotTest, CoopNoShootToFortressWall)

// TEST_F(BotTest, NoShootToWater)
// TEST_F(BotTest, NoShootToBush)
// TEST_F(BotTest, NoShootToIce)