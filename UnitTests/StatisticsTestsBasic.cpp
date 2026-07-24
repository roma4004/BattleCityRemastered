#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/GameStatistics.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/SteelWall.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/Player.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include <memory>

class StatisticsTest : public testing::Test
{
	using buuid = boost::uuids::uuid;

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<GameStatistics> _statistics{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<BonusSpawner> _bonusSpawner{nullptr};
	GameConfig _gameConfig{"", true};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	int _tankHealth{1};
	int _bulletHealth{1};
	float _tankSize{};
	float _tankSpeed{142.f};
	double _deltaTimeOneFrame{1.f / 60.f};
	BulletCalibre _calibre{.speed = 300.f, .damage = 1, .damageRadius = 12.0, .tier = 1u, .size{.x = 6.f, .y = 5.f}};
	buuid _uuid{};
	GameMode _gameMode{GameMode::OnePlayer};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _gameConfig);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, &_allObjects, _gameConfig);
		_statistics = std::make_shared<GameStatistics>(_events);
		const float gridSize = static_cast<float>(_gameConfig.windowSize.y) / 50.f;
		_tankSize = gridSize * 3.f;// for better turns

		_allObjects.reserve(5);
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

TEST_F(StatisticsTest, PlayerOneHitByEnemy)
{
	// Spawn Player1
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	std::string name{"Bullet1"};
	std::string fraction{"EnemyTeam"};
	std::string author{"Enemy1"};
	const ObjRectangle rect{.x = _tankSize / 2.f, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::UP,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetPlayerOneHitByEnemyTeam(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerOneHitByEnemyTeam(), 1);
}

TEST_F(StatisticsTest, PlayerOneHitByFriend)
{
	// Spawn Player1
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player2"};
	const ObjRectangle rect{.x = _tankSize / 2.f, .y = _tankSize + 1.f, .w = _calibre.size.x, .h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::UP,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetPlayerOneHitFriendlyFire(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerOneHitFriendlyFire(), 1);
}

TEST_F(StatisticsTest, PlayerTwoHitByEnemy)
{
	// Spawn Player2
	const ObjRectangle rectPlayer2{.x = _tankSize + 1.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player2 =
			TestUtils::CreateTank<Player>(
					rectPlayer2, _tankHealth, _uuid, "Player2", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player2);

	std::string name{"Bullet1"};
	std::string fraction{"EnemyTeam"};
	std::string author{"Enemy1"};
	const ObjRectangle rect{.x = _tankSize + _tankSize / 2.f,
							.y = _tankSize,
							.w = _calibre.size.x,
							.h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::UP,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetPlayerTwoHitByEnemyTeam(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerTwoHitByEnemyTeam(), 1);
}

TEST_F(StatisticsTest, PlayerTwoHitByFriend)
{
	// Spawn Player2
	const ObjRectangle rectPlayer2{.x = _tankSize + 1.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player2 =
			TestUtils::CreateTank<Player>(
					rectPlayer2, _tankHealth, _uuid, "Player2", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player2);

	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	const ObjRectangle rect{.x = _tankSize + _tankSize / 2.f,
							.y = _tankSize,
							.w = _calibre.size.x,
							.h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::UP,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetPlayerTwoHitFriendlyFire(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerTwoHitFriendlyFire(), 1);
}

TEST_F(StatisticsTest, PlayerOneDiedByFriend)
{
	// Spawn Player1
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player2"};
	const ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::UP,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetPlayerOneDiedByFriendlyFire(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerOneDiedByFriendlyFire(), 1);
}

TEST_F(StatisticsTest, PlayerTwoDiedByEnemy)
{
	// Spawn Player2
	const ObjRectangle rectPlayer2{.x = _tankSize + 1.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player2 =
			TestUtils::CreateTank<Player>(
					rectPlayer2, _tankHealth, _uuid, "Player2", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player2);

	std::string name{"Bullet1"};
	std::string fraction{"EnemyTeam"};
	std::string author{"Enemy1"};
	const ObjRectangle rect{.x = _tankSize + _tankSize / 2.f,
							.y = _tankSize,
							.w = _calibre.size.x,
							.h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::UP,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetPlayerDiedByEnemyTeam(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerDiedByEnemyTeam(), 1);
}

TEST_F(StatisticsTest, PlayerOneDiedByEnemy)
{
	// Spawn Player1
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	std::string name{"Bullet1"};
	std::string fraction{"EnemyTeam"};
	std::string author{"Enemy1"};
	const ObjRectangle rect{.x = _calibre.size.x, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::UP,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetPlayerDiedByEnemyTeam(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerDiedByEnemyTeam(), 1);
}

TEST_F(StatisticsTest, PlayerTwoDiedByFriend)
{
	// Spawn Player2
	const ObjRectangle rectPlayer2{.x = _tankSize + 1.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player2 =
			TestUtils::CreateTank<Player>(
					rectPlayer2, _tankHealth, _uuid, "Player2", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player2);

	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	const ObjRectangle rect{.x = _tankSize + _tankSize / 2.f,
							.y = _tankSize,
							.w = _calibre.size.x,
							.h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::UP,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetPlayerTwoDiedByFriendlyFire(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetPlayerTwoDiedByFriendlyFire(), 1);
}

TEST_F(StatisticsTest, EnemyHitByFriend)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 2.f + 2.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	std::string name{"Bullet1"};
	std::string fraction{"EnemyTeam"};
	std::string author{"Enemy2"};
	const ObjRectangle rect{.x = _tankSize * 2.f + 2.f + _tankSize / 2.f,
							.y = _tankSize,
							.w = _calibre.size.x,
							.h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::UP,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetEnemyHitByFriendlyFire(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetEnemyHitByFriendlyFire(), 1);
}

TEST_F(StatisticsTest, EnemyHitByPlayerOne)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 2.f + 2.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	const ObjRectangle rect{.x = _tankSize * 2.f + 2.f + _tankSize / 2.f,
							.y = _tankSize,
							.w = _calibre.size.x,
							.h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::UP,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetEnemyHitByPlayerOne(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetEnemyHitByPlayerOne(), 1);
}

TEST_F(StatisticsTest, EnemyHitByPlayerTwo)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 2.f + 2.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player2"};
	const ObjRectangle rect{.x = _tankSize * 2.f + 2.f + _tankSize / 2.f,
							.y = _tankSize + 1,
							.w = _calibre.size.x,
							.h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::UP,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetEnemyHitByPlayerTwo(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetEnemyHitByPlayerTwo(), 1);
}

TEST_F(StatisticsTest, EnemyDiedByFriend)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 2.f + 2.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	std::string name{"Bullet1"};
	std::string fraction{"EnemyTeam"};
	std::string author{"Enemy2"};
	const ObjRectangle rect{.x = _tankSize * 2.f + 2.f + _tankSize / 2.f,
							.y = _tankSize,
							.w = _calibre.size.x,
							.h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::UP,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetEnemyDiedByFriendlyFire(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetEnemyDiedByFriendlyFire(), 1);
}

TEST_F(StatisticsTest, EnemyDiedByPlayerOne)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 2.f + 2.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	const ObjRectangle rect{.x = _tankSize * 2.f + 2.f + _tankSize / 2.f,
							.y = _tankSize + 1,
							.w = _calibre.size.x,
							.h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::UP,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetEnemyDiedByPlayerOne(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetEnemyDiedByPlayerOne(), 1);
}

TEST_F(StatisticsTest, EnemyDiedByPlayerTwo)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 2.f + 2.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player2"};
	const ObjRectangle rect{.x = _tankSize * 2.f + 2.f + _tankSize / 2.f,
							.y = _tankSize,
							.w = _calibre.size.x,
							.h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::UP,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetEnemyDiedByPlayerTwo(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetEnemyDiedByPlayerTwo(), 1);
}

TEST_F(StatisticsTest, BulletHitByPlayerTwo)
{
	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	const ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::DOWN,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	std::string name2{"Bullet2"};
	std::string fraction2{"PlayerTeam"};
	std::string author2{"Player2"};
	ObjRectangle rect2{.x = 0.f, .y = _tankSize + _calibre.size.y + 1.f, .w = _calibre.size.x, .h = _calibre.size.y};
	BaseObjProperty baseObjProperty2{
			.rect = rect2,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name2),
			.fraction = std::move(fraction2)};
	PawnProperty pawnProperty2{
			.baseObjProperty = std::move(baseObjProperty2),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::UP,
			.gameMode = _gameMode};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty2), _gameConfig, _calibre, std::move(author2),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 1);
}

TEST_F(StatisticsTest, BrickWallDiedByEnemy)
{
	ObjRectangle brickWallRect{.x = 0.f,
							   .y = _tankSize + _calibre.size.y + 1,
							   .w = _calibre.size.x,
							   .h = _calibre.size.y};

	_allObjects.emplace_back(std::make_shared<BrickWall>(brickWallRect, _events, _uuid, _gameMode));

	std::string name{"Bullet1"};
	std::string fraction{"EnemyTeam"};
	std::string author{"Enemy1"};
	const ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::DOWN,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetBrickWallDiedByEnemyTeam(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBrickWallDiedByEnemyTeam(), 1);
}

TEST_F(StatisticsTest, BrickWallDiedByPlayerOne)
{
	ObjRectangle brickWallRect{.x = 0.f,
							   .y = _tankSize + _calibre.size.y + 1,
							   .w = _calibre.size.x,
							   .h = _calibre.size.y};

	_allObjects.emplace_back(std::make_shared<BrickWall>(brickWallRect, _events, _uuid, _gameMode));

	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	const ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::DOWN,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetBrickWallDiedByPlayerOne(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBrickWallDiedByPlayerOne(), 1);
}

TEST_F(StatisticsTest, BrickDiedByPlayerTwo)
{
	ObjRectangle brickRect{.x = 0.f, .y = _tankSize + _calibre.size.y + 1, .w = _calibre.size.x, .h = _calibre.size.y};

	_allObjects.emplace_back(std::make_shared<BrickWall>(brickRect, _events, _uuid, _gameMode));

	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player2"};
	const ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::DOWN,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetBrickWallDiedByPlayerTwo(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBrickWallDiedByPlayerTwo(), 1);
}

TEST_F(StatisticsTest, SteelWallDiedByEnemy)
{
	ObjRectangle brickWallRect{.x = 0.f,
							   .y = _tankSize + _calibre.size.y + 1,
							   .w = _calibre.size.x,
							   .h = _calibre.size.y};

	_allObjects.emplace_back(std::make_shared<SteelWall>(brickWallRect, _events, _uuid, _gameMode));

	std::string name{"Bullet1"};
	std::string fraction{"EnemyTeam"};
	std::string author{"Enemy1"};
	const ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 3u,
			.speed = _calibre.speed,
			.dir = Direction::DOWN,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};
	_calibre.tier = 3u;

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetSteelWallDiedByEnemyTeam(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetSteelWallDiedByEnemyTeam(), 1);
}

TEST_F(StatisticsTest, SteelWallDiedByPlayerOne)
{
	ObjRectangle brickWallRect{.x = 0.f,
							   .y = _tankSize + _calibre.size.y + 1,
							   .w = _calibre.size.x,
							   .h = _calibre.size.y};

	_allObjects.emplace_back(std::make_shared<SteelWall>(brickWallRect, _events, _uuid, _gameMode));

	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	const ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 3u,
			.speed = _calibre.speed,
			.dir = Direction::DOWN,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};
	_calibre.tier = 3u;

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetSteelWallDiedByPlayerOne(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetSteelWallDiedByPlayerOne(), 1);
}

TEST_F(StatisticsTest, SteelDiedByPlayerTwo)
{
	ObjRectangle brickRect{.x = 0.f, .y = _tankSize + _calibre.size.y + 1, .w = _calibre.size.x, .h = _calibre.size.y};

	_allObjects.emplace_back(std::make_shared<SteelWall>(brickRect, _events, _uuid, _gameMode));

	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player2"};
	const ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 3u,
			.speed = _calibre.speed,
			.dir = Direction::DOWN,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};
	_calibre.tier = 3u;

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetSteelWallDiedByPlayerTwo(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetSteelWallDiedByPlayerTwo(), 1);
}

TEST_F(StatisticsTest, BulletHitBulletByEnemyAndByEnemy)
{
	std::string name{"Bullet1"};
	std::string fraction{"EnemyTeam"};
	std::string author{"Enemy1"};
	const ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::DOWN,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(
					std::move(pawnProperty), _gameConfig, _calibre, std::move(author), enableByDefault));

	std::string name2{"Bullet2"};
	std::string fraction2{"EnemyTeam"};
	std::string author2{"Enemy2"};
	ObjRectangle rect2{.x = 0.f, .y = _tankSize + _calibre.size.y + 1.f, .w = _calibre.size.x, .h = _calibre.size.y};
	BaseObjProperty baseObjProperty2{
			.rect = rect2,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name2),
			.fraction = std::move(fraction2)};
	PawnProperty pawnProperty2{
			.baseObjProperty = std::move(baseObjProperty2),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::UP,
			.gameMode = _gameMode};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty2), _gameConfig, _calibre, std::move(author2),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 2);
}

TEST_F(StatisticsTest, BulletHitBulletPlayerOneAndByPlayerTwo)
{
	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	const ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::DOWN,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	std::string name2{"Bullet2"};
	std::string fraction2{"PlayerTeam"};
	std::string author2{"Player2"};
	ObjRectangle rect2{.x = 0.f, .y = _tankSize + _calibre.size.y + 1.f, .w = _calibre.size.x, .h = _calibre.size.y};
	BaseObjProperty baseObjProperty2{
			.rect = rect2,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name2),
			.fraction = std::move(fraction2)};
	PawnProperty pawnProperty2{
			.baseObjProperty = std::move(baseObjProperty2),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::UP,
			.gameMode = _gameMode};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty2), _gameConfig, _calibre, std::move(author2),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 1);
}

TEST_F(StatisticsTest, BulletHitBulletByEnemyAndByPlayerOne)
{
	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player1"};
	const ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::DOWN,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	std::string name2{"Bullet2"};
	std::string fraction2{"EnemyTeam"};
	std::string author2{"Enemy1"};
	ObjRectangle rect2{.x = 0.f, .y = _tankSize + _calibre.size.y + 1.f, .w = _calibre.size.x, .h = _calibre.size.y};
	BaseObjProperty baseObjProperty2{
			.rect = rect2,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name2),
			.fraction = std::move(fraction2)};
	PawnProperty pawnProperty2{
			.baseObjProperty = std::move(baseObjProperty2),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::UP,
			.gameMode = _gameMode};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty2), _gameConfig, _calibre, std::move(author2),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 0);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 1);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerOne(), 1);
}

TEST_F(StatisticsTest, BulletHitBulletByEnemyAndByPlayerTwo)
{
	std::string name{"Bullet1"};
	std::string fraction{"PlayerTeam"};
	std::string author{"Player2"};
	const ObjRectangle rect{.x = 0.f, .y = _tankSize, .w = _calibre.size.x, .h = _calibre.size.y};
	BaseObjProperty baseObjProperty{
			.rect = rect,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name),
			.fraction = std::move(fraction)};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::DOWN,
			.gameMode = _gameMode};
	constexpr bool enableByDefault{true};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty), _gameConfig, _calibre, std::move(author),
									 enableByDefault));

	std::string name2{"Bullet2"};
	std::string fraction2{"EnemyTeam"};
	std::string author2{"Enemy1"};
	ObjRectangle rect2{.x = 0.f, .y = _tankSize + _calibre.size.y + 1.f, .w = _calibre.size.x, .h = _calibre.size.y};
	BaseObjProperty baseObjProperty2{
			.rect = rect2,
			.health = _bulletHealth,
			.uuid = _uuid,
			.name = std::move(name2),
			.fraction = std::move(fraction2)};
	PawnProperty pawnProperty2{
			.baseObjProperty = std::move(baseObjProperty2),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1u,
			.speed = _calibre.speed,
			.dir = Direction::UP,
			.gameMode = _gameMode};

	_allObjects.emplace_back(
			std::make_shared<Bullet>(std::move(pawnProperty2), _gameConfig, _calibre, std::move(author2),
									 enableByDefault));

	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 0);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBulletHitByEnemy(), 1);
	EXPECT_EQ(_statistics->GetBulletHitByPlayerTwo(), 1);
}

// Check that tank can pick up a random bonus with statistic count
TEST_F(StatisticsTest, BonusPickUpByEnemyCount)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	_bonusSpawner->SpawnRandomBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize});
	_bonusSpawner->SpawnRandomBonus({.x = _tankSize + 1.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize});

	EXPECT_EQ(_statistics->GetBonusPickupByEnemyTeam(), 0);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerTwo(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBonusPickupByEnemyTeam(), 1);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerTwo(), 0);
}

// Check that tank can pick up a random bonus with statistic count
TEST_F(StatisticsTest, BonusNotPickUpByEnemyNotCount)
{
	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	_bonusSpawner->SpawnRandomBonus({.x = 0.f, .y = _tankSize * 2 + 1.f, .w = _tankSize, .h = _tankSize});
	_bonusSpawner->SpawnRandomBonus({.x = _tankSize * 2 + 1.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize});

	EXPECT_EQ(_statistics->GetBonusPickupByEnemyTeam(), 0);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerTwo(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBonusPickupByEnemyTeam(), 0);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerTwo(), 0);
}

// Check that tank can pick up a random bonus with statistic count
TEST_F(StatisticsTest, BonusPickUpByPlayerOneCount)
{
	// Spawn Player1
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	_bonusSpawner->SpawnRandomBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize});
	constexpr bool isPressed{true};
	_events->EmitEvent("P1_Move_Down", isPressed);

	EXPECT_EQ(_statistics->GetBonusPickupByEnemyTeam(), 0);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerTwo(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBonusPickupByEnemyTeam(), 0);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerOne(), 1);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerTwo(), 0);
}

// Check that tank can pick up a random bonus with statistic count
TEST_F(StatisticsTest, BonusNotPickUpByPlayerOneNotCount)
{
	// Spawn Player1
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);
	constexpr bool isPressed{true};
	_events->EmitEvent("P1_Move_Up", isPressed);

	_bonusSpawner->SpawnRandomBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize});

	EXPECT_EQ(_statistics->GetBonusPickupByEnemyTeam(), 0);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerTwo(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBonusPickupByEnemyTeam(), 0);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerTwo(), 0);
}

// Check that tank can pick up a random bonus with statistic count
TEST_F(StatisticsTest, BonusPickUpByPlayerTwoCount)
{
	// Spawn Player2
	const ObjRectangle rectPlayer2{.x = _tankSize + 1.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player2 =
			TestUtils::CreateTank<Player>(
					rectPlayer2, _tankHealth, _uuid, "Player2", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player2);
	constexpr bool isPressed{true};
	_events->EmitEvent("P2_Move_Down", isPressed);

	_bonusSpawner->SpawnRandomBonus({.x = _tankSize + 1.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize});

	EXPECT_EQ(_statistics->GetBonusPickupByEnemyTeam(), 0);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerTwo(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBonusPickupByEnemyTeam(), 0);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerTwo(), 1);
}

// Check that tank can pick up a random bonus with statistic count
TEST_F(StatisticsTest, BonusNotPickUpByPlayerTwoNotCount)
{
	// Spawn Player2
	const ObjRectangle rectPlayer2{.x = _tankSize + 1.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player2 =
			TestUtils::CreateTank<Player>(
					rectPlayer2, _tankHealth, _uuid, "Player2", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player2);
	constexpr bool isPressed{true};
	_events->EmitEvent("P2_Move_Up", isPressed);

	_bonusSpawner->SpawnRandomBonus({.x = _tankSize + 1.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize});

	EXPECT_EQ(_statistics->GetBonusPickupByEnemyTeam(), 0);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerTwo(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_statistics->GetBonusPickupByEnemyTeam(), 0);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerOne(), 0);
	EXPECT_EQ(_statistics->GetBonusPickupByPlayerTwo(), 0);
}
