#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/TankSpawner.h"
#include "components/managers/DelayedSpawnManager.h"
#include "components/managers/GameStateManager.h"
#include "entities/obstacles/EagleTile.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/Player.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/TankType.h"
#include "utils/UuidUtils.h"
#include "gtest/gtest.h"
#include <memory>
#include <boost/uuid/random_generator.hpp>

class GameStateManagerTest : public testing::Test
{
	using buuid = boost::uuids::uuid;

protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<GameStateManager> _stateManager{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<DelayedSpawnManager> _spawnDelayManager{nullptr};
	GameConfig _gameConfig{"", true};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	int _tankHealth{100};
	float _tankSize{};
	float _tankSpeed{142};
	float _gridSize{};
	double _deltaTimeOneFrame{1.f / 60.f};
	std::string _name = "Player1";
	buuid _uuid{};
	GameMode _gameMode{GameMode::OnePlayer};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _gameConfig);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, &_allObjects, _gameConfig);
		_stateManager = std::make_shared<GameStateManager>(_events);
		_tankSpawner = std::make_shared<TankSpawner>(_gameConfig, &_allObjects, _events);
		_spawnDelayManager = std::make_shared<DelayedSpawnManager>(_events);
		_gridSize = static_cast<float>(_gameConfig.windowSize.y) / 50.f;
		_tankSize = _gridSize * 3;// for better turns

		_allObjects.reserve(4);
	}

	void TearDown() override
	{
		// Deinitialization or some cleanup operations
	}
};

// Check that Player's team can win
TEST_F(GameStateManagerTest, PlayerTeamWon)
{
	bool isGameWon{false};

	std::vector<std::pair<int, boost::uuids::uuid>> howManySpawnCounters;
	howManySpawnCounters.reserve(4);
	_events->AddListener("TankSpawn", _name, [&howManySpawnCounters](const boost::uuids::uuid& uuid)
	{
		const auto it = std::ranges::find_if(howManySpawnCounters,
											 [&uuid](const std::pair<int, boost::uuids::uuid>& p)
											 {
												 return p.second == uuid;
											 });

		if (it != howManySpawnCounters.end())
		{
			it->first++;// Increment count if UUID found
		}
		else
		{
			howManySpawnCounters.emplace_back(1, uuid);// Add new entry if UUID not found
		}
	});

	std::vector<std::pair<int, boost::uuids::uuid>> howManyDiedCounters;
	howManyDiedCounters.reserve(4);
	_events->AddListener("TankDied", _name, [&howManyDiedCounters](const boost::uuids::uuid& uuid)
	{
		const auto it = std::ranges::find_if(howManyDiedCounters,
											 [&uuid](const std::pair<int, boost::uuids::uuid>& p)
											 {
												 return p.second == uuid;
											 });

		if (it != howManyDiedCounters.end())
		{
			it->first++;// Increment count if UUID found
		}
		else
		{
			howManyDiedCounters.emplace_back(1, uuid);// Add new entry if UUID not found
		}
	});

	_events->AddListener("PlayersTeamIsWon", _name, [&isGameWon]()
	{
		isGameWon = true;
	});

	EXPECT_EQ(_tankSpawner->GetEnemyRespawnCount(), 20);
	EXPECT_EQ(_tankSpawner->GetPlayerOneRespawnCount(), 3);
	EXPECT_EQ(_tankSpawner->GetPlayerTwoRespawnCount(), 3);
	EXPECT_FALSE(isGameWon);
	for (int i = 0; i < 5; ++i)
	{
		_allObjects.clear();
		EXPECT_EQ(_allObjects.size(), 0);

		_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::ENEMY1));
		_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::ENEMY2));
		_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::ENEMY3));
		_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::ENEMY4));

		EXPECT_EQ(_tankSpawner->GetEnemyRespawnCount(), 20 - i * 4);
		_tankSpawner->RespawnTanks(true);
		EXPECT_EQ(_allObjects.size(), 4);
		std::cout << "End of respawn round" << (i + 1) << '\n';
	}

	_allObjects.clear();

	for (const auto [spawnCount, uuid]: howManySpawnCounters)
	{
		std::cout << "UUID: " << UuidUtils::GetStringUuid(uuid) << ", Count spawn: " << spawnCount << '\n';
	}

	for (const auto [diedCount, uuid]: howManyDiedCounters)
	{
		std::cout << "UUID: " << UuidUtils::GetStringUuid(uuid) << ", Count died: " << diedCount << '\n';
	}

	EXPECT_EQ(_tankSpawner->GetEnemyRespawnCount(), 0);
	EXPECT_EQ(_tankSpawner->GetPlayerOneRespawnCount(), 3);
	EXPECT_EQ(_tankSpawner->GetPlayerTwoRespawnCount(), 3);
	EXPECT_TRUE(isGameWon);

	_events->RemoveListener("PlayersTeamIsWon", _name);
	_events->RemoveListener("TankSpawn", _name);
	_events->RemoveListener("TankDied", _name);
}

// Check that Player's team can win with enemy extra life
TEST_F(GameStateManagerTest, PlayerTeamWonWithEnemyExtraLife)
{
	bool isGameWon{false};

	std::vector<std::pair<int, boost::uuids::uuid>> howManySpawnCounters;
	howManySpawnCounters.reserve(4);
	_events->AddListener("TankSpawn", _name, [&howManySpawnCounters](const boost::uuids::uuid& uuid)
	{
		const auto it = std::ranges::find_if(howManySpawnCounters,
											 [&uuid](const std::pair<int, boost::uuids::uuid>& p)
											 {
												 return p.second == uuid;
											 });

		if (it != howManySpawnCounters.end())
		{
			it->first++;// Increment count if UUID found
		}
		else
		{
			howManySpawnCounters.emplace_back(1, uuid);// Add new entry if UUID not found
		}
	});

	std::vector<std::pair<int, boost::uuids::uuid>> howManyDiedCounters;
	howManyDiedCounters.reserve(4);
	_events->AddListener("TankDied", _name, [&howManyDiedCounters](const boost::uuids::uuid& uuid)
	{
		const auto it = std::ranges::find_if(howManyDiedCounters,
											 [&uuid](const std::pair<int, boost::uuids::uuid>& p)
											 {
												 return p.second == uuid;
											 });

		if (it != howManyDiedCounters.end())
		{
			it->first++;// Increment count if UUID found
		}
		else
		{
			howManyDiedCounters.emplace_back(1, uuid);// Add new entry if UUID not found
		}
	});

	_events->AddListener("PlayersTeamIsWon", _name, [&isGameWon]()
	{
		isGameWon = true;
	});

	// Spawn Enemy
	const ObjRectangle rectEnemy{.x = _tankSize * 3.f, .y = _tankSize * 3.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Enemy> enemyBot =
			TestUtils::CreateTank<Enemy>(
					rectEnemy, _tankHealth, _uuid, "Enemy1", "EnemyTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::DOWN, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(enemyBot);

	// Spawn bonus extra life
	_bonusSpawner->SpawnBonus(
			{.x = _tankSize * 3.f, .y = _tankSize * 3.f + _tankSize + 1.f, .w = _tankSize, .h = _tankSize},
			BonusType::Tank);

	//let enemy pick up
	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	auto enemyRespawnRemain{_tankSpawner->GetEnemyRespawnCount()};
	EXPECT_EQ(enemyRespawnRemain, 21);

	for (int i = 0; i < 4; ++i)
	{
		_allObjects.clear();
		EXPECT_EQ(_allObjects.size(), 0);

		_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::ENEMY1));
		//TODO: non necessary to do every time
		_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::ENEMY2));
		_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::ENEMY3));
		_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::ENEMY4));

		EXPECT_EQ(_tankSpawner->GetEnemyRespawnCount(), 21 - i * 4);
		_tankSpawner->RespawnTanks(true);
		EXPECT_EQ(_allObjects.size(), 4);
		std::cout << "End of respawn round" << (i + 1) << " with remain enemy respawn" << _tankSpawner->
				GetEnemyRespawnCount() << '\n';
	}

	_allObjects.clear();
	EXPECT_EQ(_allObjects.size(), 0);
	EXPECT_EQ(_tankSpawner->GetEnemyRespawnCount(), 5);

	EXPECT_FALSE(isGameWon);//Check that we still not win

	std::cout << "spawn extra life tank" << '\n';
	_tankSpawner->RespawnTanks(true);//spawn 4 enemies
	EXPECT_EQ(_allObjects.size(), 4);
	_allObjects.pop_back();//remove one enemy tank	
	_tankSpawner->RespawnTanks(true);//spawn use extra life
	EXPECT_EQ(_allObjects.size(), 4);
	_allObjects.clear();// remove all 4 enemy tank

	for (const auto [spawnCount, uuid]: howManySpawnCounters)
	{
		std::cout << "UUID: " << UuidUtils::GetStringUuid(uuid) << ", Count spawn: " << spawnCount << '\n';
	}

	for (const auto [diedCount, uuid]: howManyDiedCounters)
	{
		std::cout << "UUID: " << UuidUtils::GetStringUuid(uuid) << ", Count died: " << diedCount << '\n';
	}

	EXPECT_EQ(_tankSpawner->GetEnemyRespawnCount(), 0);
	EXPECT_EQ(_tankSpawner->GetPlayerOneRespawnCount(), 3);
	EXPECT_EQ(_tankSpawner->GetPlayerTwoRespawnCount(), 3);
	EXPECT_TRUE(isGameWon);

	_events->RemoveListener("PlayersTeamIsWon", _name);
	_events->RemoveListener("TankSpawn", _name);
	_events->RemoveListener("TankDied", _name);
}

// Player team lose with broken base
TEST_F(GameStateManagerTest, PlayerTeamLoseWithBrokenBase)
{
	_events->EmitEvent("GameModeChangedTo", GameMode::OnePlayer);
	bool isGameLose{false};
	_events->AddListener("EnemiesTeamIsWon", _name, [&isGameLose]() { isGameLose = true; });

	EXPECT_EQ(_tankSpawner->GetPlayerOneRespawnCount(), 3);
	_tankSpawner->RespawnTanks(true);
	_allObjects.emplace_back(std::make_shared<EagleTile>(ObjRectangle{}, _events, _uuid, GameMode::OnePlayer));
	EXPECT_EQ(_tankSpawner->GetPlayerOneRespawnCount(), 2);

	EXPECT_FALSE(isGameLose);

	_allObjects.pop_back();// remove eagle
	EXPECT_EQ(_tankSpawner->GetPlayerOneRespawnCount(), 0);
	_allObjects.pop_back();// remove player

	EXPECT_TRUE(isGameLose);

	_events->RemoveListener("EnemiesTeamIsWon", _name);
}

// Player team lose with three deaths in a row
TEST_F(GameStateManagerTest, PlayerTeamLoseWithThreeDeath)
{
	bool isGameLose{false};
	_events->AddListener("EnemiesTeamIsWon", _name, [&isGameLose]()
	{
		isGameLose = true;
	});

	EXPECT_FALSE(isGameLose);

	EXPECT_EQ(_tankSpawner->GetPlayerOneRespawnCount(), 3);
	for (int i = 0; i < 3; ++i)
	{
		_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::PLAYER1));
		_tankSpawner->RespawnTanks(true);
		_allObjects.pop_back();
	}

	EXPECT_EQ(_tankSpawner->GetPlayerOneRespawnCount(), 0);
	EXPECT_TRUE(isGameLose);

	_events->RemoveListener("EnemiesTeamIsWon", _name);
}

// Player team lose with four deaths with extra life
TEST_F(GameStateManagerTest, PlayerTeamLoseWithExtraLifeDeath)
{
	const ObjRectangle rectPlayer{.x = 0.f, .y = 0.f, .w = _tankSize, .h = _tankSize};
	std::shared_ptr<Player> player =
			TestUtils::CreateTank<Player>(
					rectPlayer, _tankHealth, _uuid, "Player1", "PlayerTeam", &_allObjects, _events, 1u, _tankSpeed,
					Direction::UP, _gameMode, _bulletPool, _gameConfig);
	_allObjects.emplace_back(player);

	bool isGameLose{false};
	constexpr bool isPressed{true};
	_events->EmitEvent("P1_Move_Down", isPressed);

	_events->AddListener("EnemiesTeamIsWon", _name, [&isGameLose]()
	{
		isGameLose = true;
	});

	// Spawn bonus extra life
	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, BonusType::Tank);

	EXPECT_EQ(_tankSpawner->GetPlayerOneRespawnCount(), 3);

	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_tankSpawner->GetPlayerOneRespawnCount(), 4);
	for (size_t i = 0u; i < 3u; ++i)
	{
		_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::PLAYER1));
		_tankSpawner->RespawnTanks(true);
		_allObjects.pop_back();
	}
	EXPECT_EQ(_tankSpawner->GetPlayerOneRespawnCount(), 1);

	EXPECT_FALSE(isGameLose);//Check that we still don't lose because of having extra life

	_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::PLAYER1));
	_tankSpawner->RespawnTanks(true);
	_allObjects.pop_back();

	EXPECT_EQ(_tankSpawner->GetPlayerOneRespawnCount(), 0);
	EXPECT_TRUE(isGameLose);//Check that we lose after one death after

	_events->RemoveListener("EnemiesTeamIsWon", _name);
}

// Player team lose with broken base
TEST_F(GameStateManagerTest, PlayerTeamLoseWithBrokenBaseAndExtraLife)
{
	_events->EmitEvent("GameModeChangedTo", GameMode::OnePlayer);
	bool isGameLose{false};
	_events->AddListener("EnemiesTeamIsWon", _name, [&isGameLose]() { isGameLose = true; });

	EXPECT_EQ(_tankSpawner->GetEnemyRespawnCount(), 20);
	EXPECT_EQ(_tankSpawner->GetPlayerOneRespawnCount(), 3);
	EXPECT_EQ(_tankSpawner->GetPlayerTwoRespawnCount(), 3);
	_tankSpawner->RespawnTanks(true);
	EXPECT_EQ(_tankSpawner->GetEnemyRespawnCount(), 16);
	EXPECT_EQ(_tankSpawner->GetPlayerOneRespawnCount(), 2);
	EXPECT_EQ(_tankSpawner->GetPlayerTwoRespawnCount(), 3);//game mode one player so second should not respawn

	_allObjects.emplace_back(std::make_shared<EagleTile>(ObjRectangle{}, _events, _uuid, GameMode::OnePlayer));
	_allObjects.pop_back();// remove eagle
	EXPECT_EQ(_tankSpawner->GetPlayerOneRespawnCount(), 0);
	EXPECT_EQ(_tankSpawner->GetPlayerTwoRespawnCount(), 0);

	if (const auto player = dynamic_cast<Player*>(_allObjects.back().get()))
	{
		auto [x, y] = player->GetPos();//to relative spawn above player

		// Spawn bonus extra life near player
		_bonusSpawner->SpawnBonus({.x = x, .y = y - _tankSize + 1.f, .w = _tankSize, .h = _tankSize},
								  BonusType::Tank);
	}
	else
	{
		EXPECT_FALSE(true);
	}

	constexpr bool isPressed{true};
	_events->EmitEvent("P1_Move_Up", isPressed);
	_events->EmitEvent("TickUpdate", _deltaTimeOneFrame);

	EXPECT_EQ(_tankSpawner->GetPlayerOneRespawnCount(), 1);

	_allObjects.pop_back();//remove bonus
	_allObjects.pop_back();//remove player

	EXPECT_FALSE(isGameLose);

	_events->EmitEvent("SetSlotNeedRespawn", static_cast<int>(TankType::PLAYER1));
	_tankSpawner->RespawnTanks(true);

	_allObjects.pop_back();//remove player again (last extra life)

	EXPECT_TRUE(isGameLose);

	_events->RemoveListener("EnemiesTeamIsWon", _name);
}
