#include "../headers/application/Window.h"
#include "../headers/components/BonusSpawner.h"
#include "../headers/components/BulletPool.h"
#include "../headers/components/EventSystem.h"
#include "../headers/components/TankSpawner.h"
#include "../headers/enums/BonusType.h"
#include "../headers/enums/Direction.h"
#include "../headers/enums/GameMode.h"
#include "../headers/input/InputProviderForPlayerOne.h"
#include "../headers/obstacles/FortressWall.h"
#include "../headers/pawns/Enemy.h"
#include "../headers/pawns/PawnProperty.h"
#include "../headers/pawns/Player.h"

#include "gtest/gtest.h"

#include <memory>
#include <boost/uuid/random_generator.hpp>

class BonusTestEnemy : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::unique_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<Window> _window{nullptr};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;

	int _tankHealth{100};
	int _yellow{0xeaea00};
	int _gray{0x808080};
	GameMode _gameMode{OnePlayer};
	int _bulletColor{0xffffff};
	int _bulletHealth{1};
	int _bulletDamage{1};
	double _bulletDamageRadius{12.0};
	float _tankSize{};
	float _gridSize{};
	float _tankSpeed{142};
	float _bulletSpeed{300.f};
	float _deltaTimeOneFrame{1.f / 60.f};
	boost::uuids::uuid _uuid;

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_window = std::make_shared<Window>(UPoint{.x = 800, .y = 600}, std::shared_ptr<int[]>());
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _window, _gameMode);
		_tankSpawner = std::make_shared<TankSpawner>(_window, &_allObjects, _events, _bulletPool);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, &_allObjects, _window);
		_gridSize = static_cast<float>(_window->size.y) / 50.f;
		_tankSize = _gridSize * 3;// for better turns
		std::string name = "Player1";
		std::string fraction = "PlayerTeam";
		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(_events);
		static boost::uuids::random_generator uuidGenerator;
		_uuid = uuidGenerator();

		ObjRectangle rect{.x = 0, .y = 0, .w = _tankSize, .h = _tankSize};
		BaseObjProperty baseObjProperty{rect, _gray, _tankHealth, true, _uuid, "Enemy1", "EnemyTeam"};
		PawnProperty pawnProperty{
				std::move(baseObjProperty), _window, DOWN, _tankSpeed, &_allObjects, _events, 1, _gameMode};

		_allObjects.emplace_back(std::make_shared<Enemy>(std::move(pawnProperty), _bulletPool));
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};


// NOTE: when enemy pick up shovel bonus fortressWalls hide (destroyed behaviour)

TEST_F(BonusTestEnemy, ShovelPickUpByEnemyThenFortressWallBrickHide)
{
	_allObjects.emplace_back(
			std::make_shared<FortressWall>(
					ObjRectangle{.x = _tankSize + 1.f, .y = 0, .w = _gridSize, .h = _gridSize}, _window, _events,
					&_allObjects, _uuid, _gameMode));
	const auto fortressWall = dynamic_cast<FortressWall*>(_allObjects.back().get());

	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor, Shovel);

	EXPECT_TRUE(fortressWall->IsBrickWall());
	EXPECT_NE(fortressWall->GetHealth(), 0);

	_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

	EXPECT_TRUE(fortressWall->IsBrickWall());
	EXPECT_EQ(fortressWall->GetHealth(), 0);
}

// NOTE: when player pick up shovel bonus fortressWalls become steelWalls (BonusShovelSwitch)
//       then enemy pick up shovel bonus fortressWalls should hide (destroy)
TEST_F(BonusTestEnemy, ShovelPickUpByEnemyThenFortressWallSteelWallHide)
{
	_allObjects.emplace_back(
			std::make_shared<FortressWall>(
					ObjRectangle{.x = _tankSize + 1.f, .y = 0, .w = _tankSize, .h = _tankSize}, _window, _events,
					&_allObjects, _uuid, _gameMode));
	if (const auto fortressWall = dynamic_cast<FortressWall*>(_allObjects.back().get()))
	{
		EXPECT_TRUE(fortressWall->IsBrickWall());
		fortressWall->OnPlayerPickupShovel();
		//TODO: rewrite scenario P1 firstly pick up bonus shovel, then create enemy that pick up bonus shovel
		EXPECT_TRUE(fortressWall->IsSteelWall());

		_bonusSpawner->SpawnBonus(
				{.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, _bulletColor, Shovel);

		EXPECT_TRUE(fortressWall->IsSteelWall());
		EXPECT_NE(fortressWall->GetHealth(), 0);

		_events->EmitEvent<const float>("TickUpdate", _deltaTimeOneFrame);

		EXPECT_TRUE(fortressWall->IsBrickWall());
		EXPECT_EQ(fortressWall->GetHealth(), 0);

		return;
	}

	EXPECT_TRUE(false);
}
