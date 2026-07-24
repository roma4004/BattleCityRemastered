#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/TankSpawner.h"
#include "components/managers/BonusEffectManager.h"
#include "entities/bonuses/Bonus.h"
#include "entities/obstacles/FortressWall.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/BulletResetProperty.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/Player.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include <memory>

class BonusTestsDestroy : public testing::Test
{
	using buuid = boost::uuids::uuid;

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::unique_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<BonusEffectManager> _bonusEffectManager{nullptr};
	GameConfig _gameConfig{"", true};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	int _tankHealth{100};
	int _bulletHealth{1};
	float _tankSize{};
	float _gridSize{};
	float _tankSpeed{142};
	double _deltaTimeOneFrame{1.f / 60.f};
	BulletCalibre _calibre{.speed = 300.f, .damage = 1, .damageRadius = 12.0, .tier = 1u, .size{.x = 6.f, .y = 5.f}};
	buuid _uuid{};
	GameMode _gameMode{GameMode::OnePlayer};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _gameConfig);
		_tankSpawner = std::make_shared<TankSpawner>(_gameConfig, &_allObjects, _events);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, &_allObjects, _gameConfig);
		_bonusEffectManager = std::make_unique<BonusEffectManager>(_events);
		_gridSize = static_cast<float>(_gameConfig.windowSize.y) / 50.f;
		_tankSize = _gridSize * 3.f;// for better turns

		_allObjects.reserve(4);
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

// Check that bullet can destroy a random bonus
TEST_F(BonusTestsDestroy, BonusDestroy)
{
	std::string name{"Bullet1"};
	std::string fraction{"EnemyTeam"};
	std::string author{"Enemy1"};
	constexpr ObjRectangle rect{.x = 0.f, .y = 0.f, .w = 6.f, .h = 5.f};
	BaseObjProperty baseObjProperty{.rect = rect,
									.health = _bulletHealth,
									.uuid = _uuid,
									.name = name,
									.fraction = fraction};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _tankSpeed,
			.dir = Direction::DOWN,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, author, enableByDefault));

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
TEST_F(BonusTestsDestroy, BonusNotDestroy)
{
	std::string name{"Bullet1"};
	std::string fraction{"EnemyTeam"};
	std::string author{"Enemy1"};
	constexpr ObjRectangle rect{.x = 0.f, .y = 0.f, .w = 6.f, .h = 5.f};
	BaseObjProperty baseObjProperty{.rect = rect,
									.health = _bulletHealth,
									.uuid = _uuid,
									.name = std::move(name),
									.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _tankSpeed,
			.dir = Direction::RIGHT,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, author, enableByDefault));

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
TEST_F(BonusTestsDestroy, TimerDestroyByPlayerAndEnemyStillMove)
{
	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = 6.f, .h = 5.f};
	BaseObjProperty baseObjPropertyBullet{.rect = rectBullet,
										  .health = _bulletHealth,
										  .uuid = _uuid,
										  .name = std::move(name),
										  .fraction = std::move(fraction)};
	PawnProperty pawnPropertyBullet{
			.baseObjProperty = std::move(baseObjPropertyBullet),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _tankSpeed,
			.dir = Direction::DOWN,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnPropertyBullet), _gameConfig, _calibre, author, enableByDefault));

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = 7.f, .w = _tankSize, .h = _tankSize}, BonusType::Timer);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

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
TEST_F(BonusTestsDestroy, HelmetDestroyAndBulletStillCanDamageTank)
{
	const ObjRectangle rectPlayer{.x = _tankSize + 1.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player2", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = 6.f, .h = 5.f};
	BaseObjProperty baseObjPropertyBullet{.rect = rectBullet,
										  .health = _bulletHealth,
										  .uuid = _uuid,
										  .name = std::move(name),
										  .fraction = std::move(fraction)};
	PawnProperty pawnPropertyBullet{
			.baseObjProperty = std::move(baseObjPropertyBullet),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _tankSpeed,
			.dir = Direction::DOWN,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnPropertyBullet), _gameConfig, _calibre, author,
									 enableByDefault));

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = 7.f, .w = _tankSize, .h = _tankSize}, BonusType::Helmet);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	std::string name2{"Bullet2"};
	std::string fraction2{"EnemyTeam"};
	std::string author2{"Enemy1"};
	ObjRectangle rectBullet2{.x = _tankSize * 2 + 1.f, .y = 7.f, .w = 6.f, .h = 5.f};
	BaseObjProperty baseObjPropertyBullet2{.rect = rectBullet2,
										   .health = _bulletHealth,
										   .uuid = _uuid,
										   .name = std::move(name2),
										   .fraction = std::move(fraction2)};
	PawnProperty pawnPropertyBullet2{
			.baseObjProperty = std::move(baseObjPropertyBullet2),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _tankSpeed,
			.dir = Direction::LEFT,
			.gameMode = _gameMode};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnPropertyBullet2), _gameConfig, _calibre, author2,
									 enableByDefault));

	const int playerHealth = player->GetHealth();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_NE(playerHealth, player->GetHealth());
}


//Check that player can destroy Grenade bonus and enemies still full health
TEST_F(BonusTestsDestroy, GrenadeDestroyEnemyHealthFull)
{
	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = 6.f, .h = 5.f};
	BaseObjProperty baseObjPropertyBullet{.rect = rectBullet,
										  .health = _bulletHealth,
										  .uuid = _uuid,
										  .name = std::move(name),
										  .fraction = std::move(fraction)};
	PawnProperty pawnPropertyBullet{
			.baseObjProperty = std::move(baseObjPropertyBullet),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _tankSpeed,
			.dir = Direction::DOWN,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnPropertyBullet), _gameConfig, _calibre, author, enableByDefault));

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = 7.f, .w = _tankSize, .h = _tankSize}, BonusType::Grenade);

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
TEST_F(BonusTestsDestroy, TankDestroyNoExtraLife)
{
	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = 6.f, .h = 5.f};
	BaseObjProperty baseObjPropertyBullet{.rect = rectBullet,
										  .health = _bulletHealth,
										  .uuid = _uuid,
										  .name = std::move(name),
										  .fraction = std::move(fraction)};
	PawnProperty pawnPropertyBullet{
			.baseObjProperty = std::move(baseObjPropertyBullet),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _tankSpeed,
			.dir = Direction::DOWN,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnPropertyBullet), _gameConfig, _calibre, author, enableByDefault));

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = 7.f, .w = _tankSize, .h = _tankSize}, BonusType::Tank);

	const int playerSpawnCount = _tankSpawner->GetPlayerOneRespawnCount();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(playerSpawnCount, _tankSpawner->GetPlayerOneRespawnCount());
}

//Check that player destroys Star bonus and his tier counts remain the same
TEST_F(BonusTestsDestroy, StarDestroyTierRemainTheSame)
{
	const ObjRectangle rectPlayer{.x = 0.f, .y = _tankSize * 3.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player2", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = 6.f, .h = 5.f};
	BaseObjProperty baseObjPropertyBullet{.rect = rectBullet,
										  .health = _bulletHealth,
										  .uuid = _uuid,
										  .name = std::move(name),
										  .fraction = std::move(fraction)};
	PawnProperty pawnPropertyBullet{
			.baseObjProperty = std::move(baseObjPropertyBullet),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _tankSpeed,
			.dir = Direction::DOWN,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnPropertyBullet), _gameConfig, _calibre, author,
									 enableByDefault));

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = 7.f, .w = _tankSize, .h = _tankSize}, BonusType::Star);

	EXPECT_EQ(player->GetTier(), 1u);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(player->GetTier(), 1u);
}

//Check that player destroys Shovel bonus and fortress brick remain the same
TEST_F(BonusTestsDestroy, ShovelNotPickUpByPlayerThenfortressWallRemainTheSame)
{
	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	ObjRectangle rectBullet{.x = 0.f, .y = 0.f, .w = 6.f, .h = 5.f};
	BaseObjProperty baseObjPropertyBullet{.rect = rectBullet,
										  .health = _bulletHealth,
										  .uuid = _uuid,
										  .name = std::move(name),
										  .fraction = std::move(fraction)};
	PawnProperty pawnPropertyBullet{
			.baseObjProperty = std::move(baseObjPropertyBullet),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _tankSpeed,
			.dir = Direction::DOWN,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};
	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnPropertyBullet), _gameConfig, _calibre, author, enableByDefault));

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = 7.f, .w = _tankSize, .h = _tankSize}, BonusType::Shovel);
	const auto fortressWall =
			std::make_shared<FortressWall>(ObjRectangle{.x = _tankSize + 1.f, .y = 0, .w = _gridSize, .h = _gridSize},
										   _events, &_allObjects, _uuid, _gameMode);

	EXPECT_TRUE(fortressWall->IsBrickWall());

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_TRUE(fortressWall->IsBrickWall());
}
