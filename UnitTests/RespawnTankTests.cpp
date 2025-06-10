#include "../headers/application/Window.h"
#include "../headers/components/BulletPool.h"
#include "../headers/components/EventSystem.h"
#include "../headers/components/GameStatistics.h"
#include "../headers/components/TankSpawner.h"
#include "../headers/enums/Direction.h"
#include "../headers/enums/GameMode.h"
#include "../headers/input/InputProviderForPlayerOne.h"
#include "../headers/input/InputProviderForPlayerTwo.h"
#include "../headers/pawns/Enemy.h"
#include "../headers/pawns/PawnProperty.h"
#include "../headers/pawns/Player.h"

#include "gtest/gtest.h"

#include <memory>

class TankSpawnerTest : public testing::Test
{
	using buuid = boost::uuids::uuid;

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<GameStatistics> _statistics{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<Window> _window{nullptr};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	int _tankHealth{100};
	float _tankSize{};
	float _tankSpeed{142};
	float _bulletSpeed{300.f};
	int _yellow{0xeaea00};
	int _green{0x408000};
	int _gray{0x808080};
	std::string _name = "Player1";
	std::string _fraction = "PlayerTeam";
	std::string _name2 = "Player2";
	std::string _fraction2 = "PlayerTeam";
	GameMode _gameMode{OnePlayer};
	buuid _uuid{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_window = std::make_shared<Window>(UPoint{.x = 800, .y = 600}, std::shared_ptr<int[]>());
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _window, _gameMode);
		_statistics = std::make_shared<GameStatistics>(_events);
		_tankSpawner = std::make_shared<TankSpawner>(_window, &_allObjects, _events, _bulletPool);
		const float gridSize = static_cast<float>(_window->size.y) / 50.f;
		_tankSize = gridSize * 3;// for better turns

		std::unique_ptr<IInputProvider> inputProvider = std::make_unique<InputProviderForPlayerOne>(_events);
		std::unique_ptr<IInputProvider> inputProvider2 = std::make_unique<InputProviderForPlayerTwo>(_events);

		const ObjRectangle rect1{.x = 0, .y = 0, .w = _tankSize, .h = _tankSize};
		BaseObjProperty baseObjProperty{rect1, _yellow, _tankHealth, true, _uuid, _name, _fraction};
		PawnProperty pawnProperty{
				std::move(baseObjProperty), &_allObjects, _events, _window, _gameMode, 1, UP, _tankSpeed};

		const ObjRectangle rect2{.x = _tankSize, .y = 0, .w = _tankSize, .h = _tankSize};
		BaseObjProperty baseObjProperty2{rect2, _green, _tankHealth, true, _uuid, _name2, _fraction2};
		PawnProperty pawnProperty2{
				std::move(baseObjProperty2), &_allObjects, _events, _window, _gameMode, 1, UP, _tankSpeed};

		_allObjects.reserve(4);
		_allObjects.emplace_back(
				std::make_shared<Player>(std::move(pawnProperty), _bulletPool, std::move(inputProvider)));
		_allObjects.emplace_back(
				std::make_shared<Player>(std::move(pawnProperty2), _bulletPool, std::move(inputProvider2)));
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

TEST_F(TankSpawnerTest, EnemyOneRespawnNeededFlag)
{
	EXPECT_EQ(_tankSpawner->IsEnemyOneNeedRespawn(), false);
	_tankSpawner->SetSlotNeedRespawn(0);
	_events->EmitEvent("RespawnTanks");
	_allObjects.pop_back();

	EXPECT_EQ(_tankSpawner->IsEnemyOneNeedRespawn(), true);
}

TEST_F(TankSpawnerTest, EnemyTwoRespawnNeededFlag)
{
	EXPECT_EQ(_tankSpawner->IsEnemyTwoNeedRespawn(), false);
	_tankSpawner->SetSlotNeedRespawn(1);
	_events->EmitEvent("RespawnTanks");
	_allObjects.pop_back();

	EXPECT_EQ(_tankSpawner->IsEnemyTwoNeedRespawn(), true);
}

TEST_F(TankSpawnerTest, EnemyThreeRespawnNeededFlag)
{
	EXPECT_EQ(_tankSpawner->IsEnemyThreeNeedRespawn(), false);
	_tankSpawner->SetSlotNeedRespawn(2);
	_events->EmitEvent("RespawnTanks");
	_allObjects.pop_back();

	EXPECT_EQ(_tankSpawner->IsEnemyThreeNeedRespawn(), true);
}

TEST_F(TankSpawnerTest, EnemyFourRespawnNeededFlag)
{
	EXPECT_EQ(_tankSpawner->IsEnemyFourNeedRespawn(), false);
	_tankSpawner->SetSlotNeedRespawn(3);
	_events->EmitEvent("RespawnTanks");
	_allObjects.pop_back();

	EXPECT_EQ(_tankSpawner->IsEnemyFourNeedRespawn(), true);
}

TEST_F(TankSpawnerTest, PlayerOneDiedRespawnNeededFlag)
{
	EXPECT_EQ(_tankSpawner->IsPlayerOneNeedRespawn(), false);
	_tankSpawner->SetSlotNeedRespawn(4);
	_events->EmitEvent("RespawnTanks");
	_allObjects.pop_back();

	EXPECT_EQ(_tankSpawner->IsPlayerOneNeedRespawn(), true);
}

TEST_F(TankSpawnerTest, PlayerTwoDiedRespawnNeededFlag)
{
	EXPECT_EQ(_tankSpawner->IsPlayerTwoNeedRespawn(), false);
	_tankSpawner->SetSlotNeedRespawn(5);
	_events->EmitEvent("RespawnTanks");
	_allObjects.pop_back();

	EXPECT_EQ(_tankSpawner->IsPlayerTwoNeedRespawn(), true);
}

TEST_F(TankSpawnerTest, EnemyDiedRespawnCount)
{
	const int respawnResource = _tankSpawner->GetEnemyRespawnResource();
	_tankSpawner->SetSlotNeedRespawn(1);
	_events->EmitEvent("RespawnTanks");
	_allObjects.pop_back();

	EXPECT_GT(respawnResource, _tankSpawner->GetEnemyRespawnResource());
}

TEST_F(TankSpawnerTest, PlayerOneDiedRespawnCount)
{
	const int respawnResource = _tankSpawner->GetPlayerOneRespawnResource();
	_tankSpawner->SetSlotNeedRespawn(4);
	_events->EmitEvent("RespawnTanks");
	_allObjects.pop_back();

	EXPECT_GT(respawnResource, _tankSpawner->GetPlayerOneRespawnResource());
}

TEST_F(TankSpawnerTest, PlayerTwoDiedRespawnCount)
{
	const int respawnResource = _tankSpawner->GetPlayerTwoRespawnResource();
	_tankSpawner->SetSlotNeedRespawn(5);
	_events->EmitEvent("RespawnTanks");
	_allObjects.pop_back();

	EXPECT_GT(respawnResource, _tankSpawner->GetPlayerTwoRespawnResource());
}
