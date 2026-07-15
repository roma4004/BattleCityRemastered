#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/TankSpawner.h"
#include "components/input/InputProviderForPlayerOne.h"
#include "components/managers/BonusEffectManager.h"
#include "entities/obstacles/FortressWall.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/PawnProperty.h"
#include "entities/pawns/Player.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include <memory>

class BonusTestEnemy : public testing::Test
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
	UPoint _windowSize{.x = 800u, .y = 600u};
	int _tankHealth{100};
	// int _bulletHealth{1};
	// int _bulletDamage{1};
	// double _bulletDamageRadius{12.0};
	float _tankSize{};
	float _gridSize{};
	float _tankSpeed{142};
	// float _bulletSpeed{300.f};
	double _deltaTimeOneFrame{1.f / 60.f};
	buuid _uuid{};
	GameMode _gameMode{GameMode::OnePlayer};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _windowSize, _gameMode);
		_tankSpawner = std::make_shared<TankSpawner>(_gameConfig, &_allObjects, _events);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, &_allObjects, _gameConfig);
		_bonusEffectManager = std::make_unique<BonusEffectManager>(_events);
		_gridSize = static_cast<float>(_windowSize.y) / 50.f;
		_tankSize = _gridSize * 3;// for better turns
		// std::string name = "Player1";
		// std::string fraction = "PlayerTeam";
		// std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(_events);


		const ObjRectangle rect{.x = 0, .y = 0, .w = _tankSize, .h = _tankSize};
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
				.speed = _tankSpeed,
				.windowSize = _windowSize,
				.dir = Direction::DOWN,
				.gameMode = _gameMode};
		constexpr bool enableByDefault{true};

		_allObjects.emplace_back(std::make_shared<Enemy>(std::move(pawnProperty), _bulletPool, _gameConfig, enableByDefault));
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};


// NOTE: when the enemy picks up bonusShovel, then fortressWalls hide (destroy) brick walls around it
TEST_F(BonusTestEnemy, ShovelPickUpByEnemyThenFortressBricWallkHide)
{
	_allObjects.emplace_back(
			std::make_shared<FortressWall>(
					ObjRectangle{.x = _tankSize + 1.f, .y = 0, .w = _gridSize, .h = _gridSize}, _events, &_allObjects,
					_uuid, _gameMode));
	const auto fortressWall = dynamic_cast<const FortressWall*>(_allObjects.back().get());

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
	//creating player
	std::string name = "Player1";
	std::string fraction = "PlayerTeam";
	std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(_events);

	const ObjRectangle playerRect{.x = _tankSize * 2.f, .y = _tankSize * 2.f, .w = _tankSize, .h = _tankSize};
	BaseObjProperty baseObjProperty{.rect = playerRect,
									.health = _tankHealth,
									.uuid = _uuid,
									.name = std::move(name),
									.fraction = std::move(fraction)};
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
			std::make_shared<Player>(
					std::move(pawnProperty), _bulletPool, std::move(inputProvider), _gameConfig, enableByDefault));
	bool isPressed{true};
	_events->EmitEvent("P1_Move_Down", isPressed);

	//creating FortressWall
	const ObjRectangle fortressRect{.x = _tankSize * 3.f + 1.f, .y = _tankSize * 3.f, .w = _tankSize, .h = _tankSize};
	auto fortressWall = std::make_shared<FortressWall>(fortressRect, _events, &_allObjects, _uuid, _gameMode);
	_allObjects.emplace_back(fortressWall);

	if (fortressWall)
	{
		//creating bonuses
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
