#include "components/managers/BonusEffectManager.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/TankSpawner.h"
#include "components/input/InputProviderForPlayerOne.h"
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

class BonusTest : public testing::Test
{
	using buuid = boost::uuids::uuid;

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::unique_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<BonusEffectManager> _bonusEffectManager{nullptr};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	UPoint _windowSize{.x = 800, .y = 600};
	int _tankHealth{100};
	unsigned int _yellow{0xeaea00};
	unsigned int _gray{0x808080};
	unsigned int _bulletColor{0xffffff};
	int _bulletHealth{1};
	float _tankSize{};
	float _gridSize{};
	float _tankSpeed{142};
	double _deltaTimeOneFrame{1.f / 60.f};
	BulletCalibre _calibre{.speed = 300.f, .damage = 1, .damageRadius = 12.0, .tier = 1, .size{.x = 6.f, .y = 5.f}};
	buuid _uuid{};
	GameMode _gameMode{GameMode::OnePlayer};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _windowSize, _gameMode);
		_tankSpawner = std::make_shared<TankSpawner>(_windowSize, &_allObjects, _events);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, &_allObjects, _windowSize);
		_bonusEffectManager = std::make_unique<BonusEffectManager>(_events);
		_gridSize = static_cast<float>(_windowSize.y) / 50.f;
		_tankSize = _gridSize * 3.f;// for better turns
		std::string name = "Player1";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(_events);

		const ObjRectangle rect{.x = 0, .y = 0, .w = _tankSize, .h = _tankSize};
		BaseObjProperty baseObjProperty{.rect = rect,
										.color = _yellow,
										.health = _tankHealth,
										.uuid = _uuid,
										.name = std::move(name),
										.fraction = std::move(fraction)};
		PawnProperty pawnProperty{
				.baseObjProperty = std::move(baseObjProperty),
				.allObjects = &_allObjects,
				.events = _events,
				.tier = 1,
				.speed = _tankSpeed,
				.windowSize = _windowSize,
				.dir = Direction::UP,
				.gameMode = _gameMode};
		constexpr bool enableByDefault{true};

		_allObjects.reserve(4);
		_allObjects.emplace_back(
				std::make_shared<Player>(
						std::move(pawnProperty), _bulletPool, std::move(inputProvider), enableByDefault));
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

// Check that tank can pick up a random bonus
TEST_F(BonusTest, BonusPickUp)
{
	_bonusSpawner->SpawnRandomBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize});

	constexpr bool isPressed{true};
	_events->EmitEvent("P1_Move_Down", isPressed);

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
	if (auto player = dynamic_cast<Player*>(_allObjects.front().get()))
	{
		player->SetPos(FPoint{.x = 0.f, .y = 0.f});

		_bonusSpawner->SpawnRandomBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize});

		constexpr bool isPressed{true};
		_events->EmitEvent("P1_Move_Up", isPressed);

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

		return;
	}

	EXPECT_TRUE(false);
}

// Check that player can pick up Timer bonus and freeze enemy
TEST_F(BonusTest, TimerPickUpEnemyCantMove)
{
	if (dynamic_cast<Player*>(_allObjects.front().get()))
	{
		_bonusSpawner->SpawnBonus(
				{.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor, BonusType::Timer);
		constexpr bool isPressed{true};
		_events->EmitEvent("P1_Move_Down", isPressed);

		ObjRectangle rect{.x = _tankSize * 2, .y = _tankSize * 2, .w = _tankSize, .h = _tankSize};
		BaseObjProperty baseObjProperty{.rect = rect,
										.color = _gray,
										.health = _tankHealth,
										.uuid = _uuid,
										.name = "Enemy1",
										.fraction = "EnemyTeam"};
		PawnProperty pawnProperty{
				.baseObjProperty = std::move(baseObjProperty),
				.allObjects = &_allObjects,
				.events = _events,
				.tier = 1,
				.speed = _tankSpeed,
				.windowSize = _windowSize,
				.dir = Direction::DOWN,
				.gameMode = _gameMode};

		constexpr bool enableByDefault{true};
		const auto enemy = std::make_shared<Enemy>(std::move(pawnProperty), _bulletPool, enableByDefault);

		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		const FPoint enemyPos = enemy->GetPos();

		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_EQ(enemyPos, enemy->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

// Check that player not pick up Timer bonus and enemies still move
TEST_F(BonusTest, TimerNotPickUpEnemyCanMove)
{
	if (dynamic_cast<Player*>(_allObjects.front().get()))
	{
		_bonusSpawner->SpawnBonus(
				{.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor, BonusType::Timer);
		constexpr bool isPressed{true};
		_events->EmitEvent("P1_Move_Up", isPressed);

		ObjRectangle rect{.x = _tankSize * 2, .y = _tankSize * 2, .w = _tankSize, .h = _tankSize};
		BaseObjProperty baseObjProperty{.rect = rect,
										.color = _gray,
										.health = _tankHealth,
										.uuid = _uuid,
										.name = "Enemy1",
										.fraction = "EnemyTeam"};
		PawnProperty pawnProperty{
				.baseObjProperty = std::move(baseObjProperty),
				.allObjects = &_allObjects,
				.events = _events,
				.tier = 1,
				.speed = _tankSpeed,
				.windowSize = _windowSize,
				.dir = Direction::DOWN,
				.gameMode = _gameMode};

		constexpr bool enableByDefault{true};
		const auto enemy = std::make_shared<Enemy>(std::move(pawnProperty), _bulletPool, enableByDefault);
		//TODO: spawn with helmet or timer effect for test instead of bonus pickup in separated test

		const FPoint enemyPos = enemy->GetPos();

		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_NE(enemyPos, enemy->GetPos());

		return;
	}

	EXPECT_TRUE(false);
}

//Check that player can pick up Helmet bonus and enemies can't damage player
TEST_F(BonusTest, HelmetPickUpAndBulletCantDamageTank)
{
	if (const auto player = dynamic_cast<Player*>(_allObjects.front().get()))
	{
		const int playerHealth = player->GetHealth();

		_bonusSpawner->SpawnBonus(
				{.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor, BonusType::Helmet);
		constexpr bool isPressed{true};
		_events->EmitEvent("P1_Move_Down", isPressed);
		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);
		if (const auto bonus = dynamic_cast<Bonus*>(_allObjects.back().get()))
		{
			EXPECT_EQ(bonus->GetIsAlive(), false);
		}
		else
		{
			EXPECT_TRUE(false);
		}

		std::string name{"Bullet1"};
		std::string fraction{"EnemyTeam"};
		std::string author{"Enemy1"};
		ObjRectangle rect{.x = _tankSize + 1.f, .y = 0.f, .w = 6.f, .h = 5.f};
		BaseObjProperty baseObjProperty{.rect = rect,
										.color = _bulletColor,
										.health = _bulletHealth,
										.uuid = _uuid,
										.name = std::move(name),
										.fraction = std::move(fraction)};
		PawnProperty pawnProperty{
				.baseObjProperty = std::move(baseObjProperty),
				.allObjects = &_allObjects,
				.events = _events,
				.tier = 1,
				.speed = _tankSpeed,
				.windowSize = _windowSize,
				.dir = Direction::LEFT,
				.gameMode = _gameMode};

		_allObjects.emplace_back(std::make_shared<Bullet>(std::move(pawnProperty), _calibre, std::move(author)));

		if (dynamic_cast<Bullet*>(_allObjects.back().get()))
		{
			_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

			EXPECT_EQ(playerHealth, player->GetHealth());

			return;
		}

		return;
	}

	EXPECT_TRUE(false);
}

//Check that player not pick up Helmet bonus and enemies can damage player
TEST_F(BonusTest, HelmetNotPickUpBulletCanDamageTank)
{
	if (const auto player = dynamic_cast<const Player*>(_allObjects.front().get()))
	{
		const int playerHealth = player->GetHealth();

		_bonusSpawner->SpawnBonus(
				{.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor, BonusType::Helmet);
		constexpr bool isPressed{true};
		_events->EmitEvent("P1_Move_Up", isPressed);
		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);
		if (const auto bonus = dynamic_cast<Bonus*>(_allObjects.back().get()))
		{
			EXPECT_TRUE(bonus->GetIsAlive());
		}
		else
		{
			EXPECT_TRUE(false);
		}

		std::string name{"Bullet1"};
		std::string fraction{"EnemyTeam"};
		std::string author{"Enemy1"};
		ObjRectangle rect{.x = _tankSize + 1.f, .y = 0.f, .w = 6.f, .h = 5.f};
		BaseObjProperty baseObjProperty{.rect = rect,
										.color = _bulletColor,
										.health = _bulletHealth,
										.uuid = _uuid,
										.name = std::move(name),
										.fraction = std::move(fraction)};
		PawnProperty pawnProperty{
				.baseObjProperty = std::move(baseObjProperty),
				.allObjects = &_allObjects,
				.events = _events,
				.tier = 1,
				.speed = _tankSpeed,
				.windowSize = _windowSize,
				.dir = Direction::LEFT,
				.gameMode = _gameMode};
		constexpr bool enableByDefault{true};

		_allObjects.emplace_back(
				std::make_shared<Bullet>(std::move(pawnProperty), _calibre, std::move(author), enableByDefault));

		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_NE(playerHealth, player->GetHealth());

		return;
	}

	EXPECT_TRUE(false);
}

//Check that player pick up Grenade bonus and enemies got zero health
TEST_F(BonusTest, GrenadePickUpEnemyHealthZero)
{
	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor,
							  BonusType::Grenade);
	constexpr bool isPressed{true};
	_events->EmitEvent("P1_Move_Down", isPressed);

	ObjRectangle rect{.x = _tankSize * 2, .y = _tankSize * 2, .w = _tankSize, .h = _tankSize};
	BaseObjProperty baseObjProperty{.rect = rect,
									.color = _gray,
									.health = _tankHealth,
									.uuid = _uuid,
									.name = "Enemy1",
									.fraction = "EnemyTeam"};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1,
			.speed = _tankSpeed,
			.windowSize = _windowSize,
			.dir = Direction::DOWN,
			.gameMode = _gameMode};

	constexpr bool enableByDefault{true};
	const auto enemy = std::make_shared<Enemy>(std::move(pawnProperty), _bulletPool, enableByDefault);

	EXPECT_EQ(enemy->GetHealth(), 100);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	if (const auto bonus = dynamic_cast<Bonus*>(_allObjects.back().get()))
	{
		EXPECT_EQ(bonus->GetIsAlive(), false);
	}
	else
	{
		EXPECT_TRUE(false);
	}

	EXPECT_EQ(enemy->GetHealth(), 0);
}

//Check that not player pick up Grenade bonus and enemies remain full health
TEST_F(BonusTest, GrenadeNotPickUpEnemyHealthFull)
{
	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor,
							  BonusType::Grenade);
	constexpr bool isPressed{true};
	_events->EmitEvent("P1_Move_Up", isPressed);

	ObjRectangle rect{.x = _tankSize * 2, .y = _tankSize * 2, .w = _tankSize, .h = _tankSize};
	BaseObjProperty baseObjProperty{.rect = rect,
									.color = _gray,
									.health = _tankHealth,
									.uuid = _uuid,
									.name = "Enemy1",
									.fraction = "EnemyTeam"};
	PawnProperty pawnProperty{
			.baseObjProperty = std::move(baseObjProperty),
			.allObjects = &_allObjects,
			.events = _events,
			.tier = 1,
			.speed = _tankSpeed,
			.windowSize = _windowSize,
			.dir = Direction::DOWN,
			.gameMode = _gameMode};

	constexpr bool enableByDefault{true};
	const auto enemy = std::make_shared<Enemy>(std::move(pawnProperty), _bulletPool, enableByDefault);

	EXPECT_EQ(enemy->GetHealth(), 100);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	if (const auto bonus = dynamic_cast<Bonus*>(_allObjects.back().get()))
	{
		EXPECT_TRUE(bonus->GetIsAlive());
	}
	else
	{
		EXPECT_TRUE(false);
	}

	EXPECT_EQ(enemy->GetHealth(), 100);
}

//Check that player pick up Tank bonus and got his extra life
TEST_F(BonusTest, TankPickUpExtraLife)
{
	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor,
							  BonusType::Tank);
	constexpr bool isPressed{true};
	_events->EmitEvent("P1_Move_Down", isPressed);

	const int playerSpawnCount = _tankSpawner->GetPlayerOneRespawnCount();
	if (const auto bonus = dynamic_cast<Bonus*>(_allObjects.back().get()))
	{
		EXPECT_TRUE(bonus->GetIsAlive());

		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_FALSE(bonus->GetIsAlive());

		EXPECT_LT(playerSpawnCount, _tankSpawner->GetPlayerOneRespawnCount());

		return;
	}

	EXPECT_TRUE(false);
}

//Check that player not pick up Tank bonus and his life count remains the same
TEST_F(BonusTest, TankNotPickUpTierTheSame)
{
	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor,
							  BonusType::Tank);
	constexpr bool isPressed{true};
	_events->EmitEvent("P1_Move_Up", isPressed);

	const int playerSpawnCount = _tankSpawner->GetPlayerOneRespawnCount();

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	if (const auto bonus = dynamic_cast<Bonus*>(_allObjects.back().get()))
	{
		EXPECT_NE(bonus->GetIsAlive(), false);
	}
	else
	{
		EXPECT_TRUE(false);
	}

	EXPECT_EQ(playerSpawnCount, _tankSpawner->GetPlayerOneRespawnCount());
}

//Check that player pick up Star bonus and his tier increased
TEST_F(BonusTest, StarPickUpTierIncrease)
{
	if (const auto player = dynamic_cast<Player*>(_allObjects.front().get()))
	{
		_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor,
								  BonusType::Star);
		constexpr bool isPressed{true};
		_events->EmitEvent("P1_Move_Down", isPressed);

		EXPECT_EQ(player->GetTier(), 1);

		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		if (const auto bonus = dynamic_cast<Bonus*>(_allObjects.back().get()))
		{
			EXPECT_EQ(bonus->GetIsAlive(), false);
		}
		else
		{
			EXPECT_TRUE(false);
		}

		EXPECT_EQ(player->GetTier(), 2);

		return;
	}

	EXPECT_TRUE(false);
}

//Check that player not pick up Star bonus and his tier remains the same
TEST_F(BonusTest, StarNotPickUpTierTheSame)
{
	if (const auto player = dynamic_cast<Player*>(_allObjects.front().get()))
	{
		_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor,
								  BonusType::Star);
		constexpr bool isPressed{true};
		_events->EmitEvent("P1_Move_Up", isPressed);

		EXPECT_EQ(player->GetTier(), 1);

		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		if (const auto bonus = dynamic_cast<Bonus*>(_allObjects.back().get()))
		{
			EXPECT_TRUE(bonus->GetIsAlive());
		}
		else
		{
			EXPECT_TRUE(false);
		}

		EXPECT_EQ(player->GetTier(), 1);

		return;
	}

	EXPECT_TRUE(false);
}

// NOTE: when player pick up shovel bonus fortressWalls become steelWalls for a while then return to regular brickWalls
//Check that player pick up Shovel bonus and Fortress wall turns into Steel wall
TEST_F(BonusTest, ShovelPickUpByPlayerThenFortressWallTurnIntoSteelWall)
{
	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor,
							  BonusType::Shovel);
	constexpr bool isPressed{true};
	_events->EmitEvent("P1_Move_Down", isPressed);

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
	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor,
							  BonusType::Shovel);
	constexpr bool isPressed{true};
	_events->EmitEvent("P1_Move_Up", isPressed);

	const auto fortressWall =
			std::make_shared<FortressWall>(ObjRectangle{.x = _tankSize + 1.f, .y = 0, .w = _gridSize, .h = _gridSize},
										   _events, &_allObjects, _uuid, _gameMode);

	EXPECT_TRUE(fortressWall->IsBrickWall());

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_TRUE(fortressWall->IsBrickWall());
}
