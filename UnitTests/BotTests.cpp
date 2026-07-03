#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/TankSpawner.h"
#include "components/managers/StateManager.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "components/managers/DelayedSpawnManager.h"
#include "entities/obstacles/BrickWall.h"
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

// Check that bot changes direction to the opponent
TEST_F(BotTest, ChangeDirectionIfOponentSeen)
{
	if (const auto coopBot = dynamic_cast<CoopBot*>(_allObjects.front().get()))
	{
		ObjRectangle rect{.x = _tankSize * 2.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
		BaseObjProperty baseObjProperty{.rect = rect,
										.health = _tankHealth,
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

// Check that bots change their direction to the bonus
TEST_F(BotTest, ChangeDirectionIfBonusSeenAndNoOneShoot)
{
	if (const auto coopBot = dynamic_cast<CoopBot*>(_allObjects.front().get()))
	{
		_bonusSpawner->SpawnRandomBonus({.x = _tankSize + 21.f, .y = 0.f, .w = _tankSize, .h = _tankSize});

		ObjRectangle rect{.x = _tankSize * 3.f + 40.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
		BaseObjProperty baseObjProperty{.rect = rect,
										.health = _tankHealth,
										.uuid = _uuid,
										.name = "Enemy1",
										.fraction = "EnemyTeam"};
		PawnProperty pawnProperty{.baseObjProperty = std::move(baseObjProperty),
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
		const size_t sizeBefore = _allObjects.size();

		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		const Direction endDirCoop = coopBot->GetDirection();
		const Direction endDirEnemy = enemy->GetDirection();

		EXPECT_EQ(sizeBefore, _allObjects.size());
		EXPECT_NE(startDirCoop, endDirCoop);
		EXPECT_NE(startDirEnemy, endDirEnemy);
		EXPECT_EQ(endDirCoop, Direction::RIGHT);
		EXPECT_EQ(endDirEnemy, Direction::LEFT);

		return;
	}

	EXPECT_TRUE(false);
}

// Check that bot does not change direction if it can't see the bonus
TEST_F(BotTest, NoChangeDirIfBonusOutsideLineOfSight)
{
	if (const auto coopBot = dynamic_cast<CoopBot*>(_allObjects.front().get()))
	{
		// Spawn a bonus diagonally, out of the bot's direct line of sight
		_bonusSpawner->SpawnRandomBonus({.x = _tankSize * 2.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize});

		const Direction startDirCoop = coopBot->GetDirection();

		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		const Direction endDirCoop = coopBot->GetDirection();

		EXPECT_EQ(startDirCoop, endDirCoop);

		return;
	}

	EXPECT_TRUE(false);
}

// Check that bot shoots when seeing an opponent
TEST_F(BotTest, ShootToOpponent)
{
	if (const auto coopBot = dynamic_cast<CoopBot*>(_allObjects.front().get()))
	{
		// Spawn enemy in line of sight
		ObjRectangle rectEnemy = {.x = 0.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize};
		BaseObjProperty baseObjProperty = {.rect = rectEnemy,
										   .health = 1,
										   .name = "Enemy1",
										   .fraction = "EnemyTeam"};
		PawnProperty pawnProperty{.baseObjProperty = baseObjProperty,
								  .allObjects = &_allObjects,
								  .events = _events,
								  .tier = 1u,
								  .speed = _tankSpeed,
								  .windowSize = _windowSize,
								  .dir = Direction::UP,
								  .gameMode = _gameMode};

		constexpr bool enableByDefault{true};
		_allObjects.emplace_back(std::make_shared<Enemy>(std::move(pawnProperty), _bulletPool, enableByDefault));

		const size_t sizeBefore = _allObjects.size();

		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_LT(sizeBefore, _allObjects.size());// Bullet should be spawned

		return;
	}

	EXPECT_TRUE(false);
}

// Check that CoopBot does not shoot at PlayerTeam members
TEST_F(BotTest, CoopNoShootToAlly)
{
	if (const auto coopBot = dynamic_cast<CoopBot*>(_allObjects.front().get()))
	{
		coopBot->SetDirection(Direction::DOWN);
		coopBot->SetPos({.x = 0.f, .y = 0.f});

		// Spawn another player-aligned bot in line of sight
		const ObjRectangle rect = {.x = 0.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize};
		const BaseObjProperty baseObjProperty = {.rect = rect,
												 .health = 1,
												 .name = "Player2",
												 .fraction = "PlayerTeam"};
		PawnProperty pawnProperty{
				.baseObjProperty = baseObjProperty,
				.allObjects = &_allObjects,
				.events = _events,
				.tier = 1u,
				.speed = _tankSpeed,
				.windowSize = _windowSize,
				.dir = Direction::UP,
				.gameMode = _gameMode};

		constexpr bool enableByDefault{true};
		_allObjects.emplace_back(std::make_shared<CoopBot>(std::move(pawnProperty), _bulletPool, enableByDefault));

		const size_t sizeBefore = _allObjects.size();

		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_EQ(sizeBefore, _allObjects.size());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that Bot can shoot at Brick wall
TEST_F(BotTest, ShootToBrick)
{
	if (const auto coopBot = dynamic_cast<CoopBot*>(_allObjects.front().get()))
	{
		const ObjRectangle rect = {.x = 0.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize};
		_allObjects.emplace_back(std::make_shared<BrickWall>(rect, _events, _uuid, _gameMode));

		const size_t sizeBefore = _allObjects.size();

		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_LT(sizeBefore, _allObjects.size());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that Bot not shoots at Brick wall if too close
TEST_F(BotTest, TooCloseToShootTheBrick)
{
	if (const auto coopBot = dynamic_cast<CoopBot*>(_allObjects.front().get()))
	{
		const ObjRectangle rect = {.x = 0.f, .y = _tankSize + 18.f, .w = _tankSize, .h = _tankSize};
		_allObjects.emplace_back(std::make_shared<BrickWall>(rect, _events, _uuid, _gameMode));

		const size_t sizeBefore = _allObjects.size();

		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_EQ(sizeBefore, _allObjects.size());

		return;
	}

	EXPECT_TRUE(false);
}

//TODO: implement other bot tests

// TEST_F(BotTest, CoopNoShootToAlly)
// TEST_F(BotTest, EnemyNoShootToAlly)

// TEST_F(BotTest, ShootToSteelWithTier)
// TEST_F(BotTest, NoShootToSteel)

// TEST_F(BotTest, EnemyShootToEagleTile)
// TEST_F(BotTest, CoopNoShootToEagleTile)

// TEST_F(BotTest, EnemyShootToFortressWall)
// TEST_F(BotTest, CoopNoShootToFortressWall)

// TEST_F(BotTest, NoShootToWater)
// TEST_F(BotTest, NoShootToBush)
// TEST_F(BotTest, NoShootToIce)

// TEST_F(BotTest, NoReactionEnemyToPlayerBehindBrickWall)
// TEST_F(BotTest, NoReactionEnemyToPlayerBehindSteelWall)
// TEST_F(BotTest, NoReactionEnemyToPlayerBehindFortressWall)
// TEST_F(BotTest, NoReactionEnemyToPlayerBehindInTheBush)
// TEST_F(BotTest, ShootReactionEnemyToPlayerBehindWater)
// TEST_F(BotTest, ShootReactionEnemyToPlayerBehindIce)
