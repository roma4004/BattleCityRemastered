#include "components/managers/BonusEffectManager.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/TankSpawner.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
// #include "input/InputProviderForPlayerOne.h"
#include "entities/obstacles/FortressWall.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/PawnProperty.h"
// #include "pawns/Player.h"
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
	std::shared_ptr<RespawnManager> _respawnManager{nullptr};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	UPoint _windowSize{.x = 800, .y = 600};
	int _tankHealth{100};
	// int _yellow{0xeaea00};
	int _gray{0x808080};
	// int _bulletColor{0xffffff};
	// int _bulletHealth{1};
	// int _bulletDamage{1};
	// double _bulletDamageRadius{12.0};
	float _tankSize{};
	float _gridSize{};
	float _tankSpeed{142};
	// float _bulletSpeed{300.f};
	float _deltaTimeOneFrame{1.f / 60.f};
	buuid _uuid{};
	GameMode _gameMode{GameMode::OnePlayer};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _windowSize, _gameMode);
		_bonusEffectManager = std::make_shared<BonusEffectManager>(_events);
		_respawnManager = std::make_shared<RespawnManager>(_events);
		_tankSpawner = std::make_shared<TankSpawner>(_windowSize, &_allObjects, _events, _bonusEffectManager,
		                                             _respawnManager);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, &_allObjects, _windowSize);
		_gridSize = static_cast<float>(_windowSize.y) / 50.f;
		_tankSize = _gridSize * 3;// for better turns
		// std::string name = "Player1";
		// std::string fraction = "PlayerTeam";
		// std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(_events);


		const ObjRectangle rect{.x = 0, .y = 0, .w = _tankSize, .h = _tankSize};
		BaseObjProperty baseObjProperty{.rect = rect, .color = _gray, .health = _tankHealth, .uuid = _uuid,
		                                .name = "Enemy1", .fraction = "EnemyTeam"};
		PawnProperty pawnProperty{
				.baseObjProperty = std::move(baseObjProperty), .allObjects = &_allObjects, .events = _events, .tier = 1,
				.speed = _tankSpeed, .windowSize = _windowSize, .dir = Direction::DOWN, .gameMode = _gameMode};
		constexpr bool enableByDefault{true};

		_allObjects.emplace_back(
				std::make_shared<Enemy>(std::move(pawnProperty), _bulletPool, BonusEffectProperty{}, enableByDefault));
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};


// NOTE: when the enemy picks up shovel bonus fortressWalls hide (destroyed behavior)

TEST_F(BonusTestEnemy, ShovelPickUpByEnemyThenFortressWallBrickHide)
{
	_allObjects.emplace_back(
			std::make_shared<FortressWall>(
					ObjRectangle{.x = _tankSize + 1.f, .y = 0, .w = _gridSize, .h = _gridSize}, _events, &_allObjects,
					_uuid, _gameMode));
	const auto fortressWall = dynamic_cast<const FortressWall*>(_allObjects.back().get());

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, 0x0, BonusType::Shovel);

	EXPECT_TRUE(fortressWall->IsBrickWall());
	EXPECT_NE(fortressWall->GetHealth(), 0);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_TRUE(fortressWall->IsBrickWall());
	EXPECT_EQ(fortressWall->GetHealth(), -1);
}

// NOTE: when player pick up shovel bonus fortressWalls become steelWalls (BonusShovelSwitch),
//       then enemy pick up shovel bonus fortressWalls should hide (destroy)
TEST_F(BonusTestEnemy, ShovelPickUpByEnemyThenFortressWallSteelWallHide)
{
	_allObjects.emplace_back(
			std::make_shared<FortressWall>(
					ObjRectangle{.x = _tankSize + 1.f, .y = 0, .w = _tankSize, .h = _tankSize}, _events, &_allObjects,
					_uuid, _gameMode));
	if (const auto fortressWall = dynamic_cast<FortressWall*>(_allObjects.back().get()))
	{
		EXPECT_TRUE(fortressWall->IsBrickWall());
		fortressWall->OnPlayerPickupShovel();
		//TODO: rewrite scenario P1 firstly pick up bonus shovel, then create enemy that pick up bonus shovel
		EXPECT_TRUE(fortressWall->IsSteelWall());

		_bonusSpawner->SpawnBonus(
				{.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, 0x0, BonusType::Shovel);

		EXPECT_TRUE(fortressWall->IsSteelWall());
		EXPECT_NE(fortressWall->GetHealth(), 0);

		_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

		EXPECT_TRUE(fortressWall->IsBrickWall());
		EXPECT_EQ(fortressWall->GetHealth(), -1);

		return;
	}

	EXPECT_TRUE(false);
}
