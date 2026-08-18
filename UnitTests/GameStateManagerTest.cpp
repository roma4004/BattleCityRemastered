#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/TimingEvents.h"
#include "components/TankSpawner.h"
#include "components/managers/DelayedSpawnManager.h"
#include "components/managers/RespawnManager.h"
#include "components/managers/GameStateManager.h"
#include "entities/obstacles/EagleTile.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/Player.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "utils/UuidUtils.h"
#include "gtest/gtest.h"
#include <iostream>
#include <memory>

class GameStateManagerTest : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<GameStateManager> _stateManager{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<RespawnManager> _respawnManager{nullptr};
	std::shared_ptr<DelayedSpawnManager> _spawnDelayManager{nullptr};
	GameConfig _gameConfig{"", true};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.f / 60.f};
	Uuid _uuid{};
	float _tankSize{};
	float _tankSpeed{142};
	float _gridSize{};
	unsigned short _tankHealth{100u};
	GameMode _gameMode{GameMode::OnePlayer};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, &_allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _gameConfig);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, &_allObjects, _gameConfig);
		_stateManager = std::make_shared<GameStateManager>(_events);
		_respawnManager = std::make_shared<RespawnManager>(_events);
		_tankSpawner = std::make_shared<TankSpawner>(_gameConfig, &_allObjects, _events);
		_spawnDelayManager = std::make_shared<DelayedSpawnManager>(_events);
		_gridSize = static_cast<float>(_gameConfig.windowSize.y) / 50.f;
		_tankSize = _gridSize * 3.f;// for better turns

		_allObjects.reserve(4u);
	}

	void TearDown() override
	{
	}
};

// Check that Player's team can win
TEST_F(GameStateManagerTest, PlayerTeamWon)
{
	bool isGameWon{false};

	std::vector<std::pair<unsigned short, Uuid>> howManySpawnCounters;
	howManySpawnCounters.reserve(4u);
	auto spawnCounterSub = _events->AddListener([&howManySpawnCounters](const TankSpawnEvent& tankSpawnEvent)
	{
		const auto& uuid = tankSpawnEvent.uuid;
		const auto it = std::ranges::find_if(howManySpawnCounters,
											 [&uuid](const std::pair<unsigned short, Uuid>& p)
											 {
												 return p.second == uuid;
											 });

		if (it != howManySpawnCounters.end())
		{
			it->first++;// Increment count if UUID found
		}
		else
		{
			howManySpawnCounters.emplace_back(1u, uuid);// Add new entry if UUID not found
		}
	});

	std::vector<std::pair<unsigned short, Uuid>> howManyDiedCounters;
	howManyDiedCounters.reserve(4u);
	auto diedCounterSub = _events->AddListener([&howManyDiedCounters](const TankDiedEvent& tankDiedEvent)
	{
		const auto& uuid = tankDiedEvent.uuid;
		const auto it = std::ranges::find_if(howManyDiedCounters,
											 [&uuid](const std::pair<unsigned short, Uuid>& p)
											 {
												 return p.second == uuid;
											 });

		if (it != howManyDiedCounters.end())
		{
			it->first++;// Increment count if UUID found
		}
		else
		{
			howManyDiedCounters.emplace_back(1u, uuid);// Add new entry if UUID not found
		}
	});

	auto gameWonSub = _events->AddListener([&isGameWon](const PlayersTeamIsWonEvent&)
	{
		isGameWon = true;
	});

	unsigned short respawnEnemyActual{20u};
	unsigned short respawnPlayerOneActual{3u};
	unsigned short respawnPlayerTwoActual{3u};
	auto respawnCountSub = _events->AddListener(
			[&respawnEnemyActual, &respawnPlayerOneActual, &respawnPlayerTwoActual](
			const RespawnCountChangedToEvent& event)
			{
				if (event.objectName == "Enemy")
				{
					respawnEnemyActual = event.respawnCount;
				}
				else if (event.objectName == "Player1")
				{
					respawnPlayerOneActual = event.respawnCount;
				}
				else if (event.objectName == "Player2")
				{
					respawnPlayerTwoActual = event.respawnCount;
				}
			});

	EXPECT_FALSE(isGameWon);
	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::OnePlayer});
	for (unsigned short i = 0u; i < 5u; ++i)
	{
		_allObjects.clear();
		EXPECT_EQ(_allObjects.size(), 0u);

		EXPECT_EQ(respawnEnemyActual, 20u - i * 4u);
		constexpr bool skipDelay{true};
		_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
		std::cout << "End of respawn round" << (i + 1u) << '\n';
	}

	_allObjects.clear();

	for (const auto& [spawnCount, uuid]: howManySpawnCounters)
	{
		std::cout << "UUID: " << UuidUtils::GetStringUuid(uuid) << ", Count spawn: " << spawnCount << '\n';
	}

	for (const auto& [diedCount, uuid]: howManyDiedCounters)
	{
		std::cout << "UUID: " << UuidUtils::GetStringUuid(uuid) << ", Count died: " << diedCount << '\n';
	}

	EXPECT_EQ(respawnEnemyActual, 0u);
	EXPECT_EQ(respawnPlayerOneActual, 0u);
	EXPECT_EQ(respawnPlayerTwoActual, 3u);
	EXPECT_TRUE(isGameWon);

}

// Check that Player's team can win with enemy extra life
TEST_F(GameStateManagerTest, PlayerTeamWonWithEnemyExtraLife)
{
	bool isGameWon{false};

	std::vector<std::pair<unsigned short, Uuid>> howManySpawnCounters;
	howManySpawnCounters.reserve(4u);
	auto spawnCounterSub = _events->AddListener([&howManySpawnCounters](const TankSpawnEvent& tankSpawnEvent)
	{
		const auto& uuid = tankSpawnEvent.uuid;
		const auto it = std::ranges::find_if(howManySpawnCounters,
											 [&uuid](const std::pair<unsigned short, Uuid>& p)
											 {
												 return p.second == uuid;
											 });

		if (it != howManySpawnCounters.end())
		{
			it->first++;// Increment count if UUID found
		}
		else
		{
			howManySpawnCounters.emplace_back(1u, uuid);// Add new entry if UUID not found
		}
	});

	std::vector<std::pair<unsigned short, Uuid>> howManyDiedCounters;
	howManyDiedCounters.reserve(4u);
	auto diedCounterSub = _events->AddListener([&howManyDiedCounters](const TankDiedEvent& tankDiedEvent)
	{
		const auto& uuid = tankDiedEvent.uuid;
		const auto it = std::ranges::find_if(howManyDiedCounters,
											 [&uuid](const std::pair<unsigned short, Uuid>& p)
											 {
												 return p.second == uuid;
											 });

		if (it != howManyDiedCounters.end())
		{
			it->first++;// Increment count if UUID found
		}
		else
		{
			howManyDiedCounters.emplace_back(1u, uuid);// Add new entry if UUID not found
		}
	});

	auto gameWonSub = _events->AddListener([&isGameWon](const PlayersTeamIsWonEvent&)
	{
		isGameWon = true;
	});

	unsigned short respawnEnemyActual{20u};
	unsigned short respawnPlayerOneActual{3u};
	unsigned short respawnPlayerTwoActual{3u};
	auto respawnCountSub = _events->AddListener(
			[&respawnEnemyActual, &respawnPlayerOneActual, &respawnPlayerTwoActual](
			const RespawnCountChangedToEvent& event)
			{
				if (event.objectName == "Enemy")
				{
					respawnEnemyActual = event.respawnCount;
				}
				else if (event.objectName == "Player1")
				{
					respawnPlayerOneActual = event.respawnCount;
				}
				else if (event.objectName == "Player2")
				{
					respawnPlayerTwoActual = event.respawnCount;
				}
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
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(respawnEnemyActual, 21u);

	constexpr bool skipDelay{true};
	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::OnePlayer});
	for (unsigned short i = 0u; i < 4u; ++i)
	{
		_allObjects.clear();
		EXPECT_EQ(_allObjects.size(), 0u);

		EXPECT_EQ(respawnEnemyActual, 21u - i * 4u);
		_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
		std::cout << "End of respawn round" << (i + 1u) << " with remain enemy respawn" << respawnEnemyActual << '\n';
	}

	_allObjects.clear();
	EXPECT_EQ(_allObjects.size(), 0u);
	EXPECT_EQ(respawnEnemyActual, 5u);

	EXPECT_FALSE(isGameWon);//Check that we still not win

	std::cout << "spawn extra life tank" << '\n';
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});//spawn 4 enemies
	EXPECT_EQ(_allObjects.size(), 4u);
	_allObjects.pop_back();//remove one enemy tank	
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});//spawn use extra life
	EXPECT_EQ(_allObjects.size(), 4u);
	_allObjects.clear();// remove all 4 enemy tank

	for (const auto& [spawnCount, uuid]: howManySpawnCounters)
	{
		std::cout << "UUID: " << UuidUtils::GetStringUuid(uuid) << ", Count spawn: " << spawnCount << '\n';
	}

	for (const auto& [diedCount, uuid]: howManyDiedCounters)
	{
		std::cout << "UUID: " << UuidUtils::GetStringUuid(uuid) << ", Count died: " << diedCount << '\n';
	}

	EXPECT_EQ(respawnEnemyActual, 0u);
	EXPECT_EQ(respawnPlayerOneActual, 0u);
	EXPECT_EQ(respawnPlayerTwoActual, 3u);
	EXPECT_TRUE(isGameWon);

}

// Player team lose with broken base
TEST_F(GameStateManagerTest, PlayerTeamLoseWithBrokenBase)
{
	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::OnePlayer});
	bool isGameLose{false};
	auto gameLoseSub = _events->AddListener([&isGameLose](const EnemiesTeamIsWonEvent&) { isGameLose = true; });

	unsigned short respawnActual{3u};
	auto respawnCountSub = _events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		respawnActual = event.respawnCount;
	});

	EXPECT_EQ(respawnActual, 3u);
	constexpr bool skipDelay{true};
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	_allObjects.emplace_back(std::make_shared<EagleTile>(ObjRectangle{}, _events, _uuid, GameMode::OnePlayer));
	EXPECT_EQ(respawnActual, 2u);

	EXPECT_FALSE(isGameLose);

	_allObjects.pop_back();// remove eagle
	EXPECT_EQ(respawnActual, 0u);
	_allObjects.pop_back();// remove player

	EXPECT_TRUE(isGameLose);

}

// Player team lose with three deaths in a row
TEST_F(GameStateManagerTest, PlayerTeamLoseWithThreeDeath)
{
	bool isGameLose{false};
	auto gameLoseSub = _events->AddListener([&isGameLose](const EnemiesTeamIsWonEvent&)
	{
		isGameLose = true;
	});

	unsigned short respawnActual{3u};
	auto respawnCountSub = _events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.objectName == "Player1")
		{
			respawnActual = event.respawnCount;
		}
	});

	EXPECT_FALSE(isGameLose);

	EXPECT_EQ(respawnActual, 3u);
	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::OnePlayer});
	for (unsigned short i = 0u; i < 3u; ++i)
	{
		constexpr bool skipDelay{true};
		_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
		_allObjects.pop_back();
	}

	EXPECT_EQ(respawnActual, 0u);
	EXPECT_TRUE(isGameLose);

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
	_events->EmitEvent(Key(std::string{"P1"}), MoveDownEvent{.isPressed = isPressed});

	auto gameLoseSub = _events->AddListener([&isGameLose](const EnemiesTeamIsWonEvent&)
	{
		isGameLose = true;
	});

	unsigned short respawnActual{3u};
	auto respawnCountSub = _events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.objectName == "Player1")
		{
			respawnActual = event.respawnCount;
		}
	});

	// Spawn bonus extra life
	_bonusSpawner->SpawnBonus({.x = 0.f, .y = _tankSize + 1.f, .w = _tankSize, .h = _tankSize}, BonusType::Tank);

	EXPECT_EQ(respawnActual, 3u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(respawnActual, 4u);
	constexpr bool skipDelay{true};
	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::OnePlayer});
	for (unsigned short i = 0u; i < 3u; ++i)
	{
		_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
		_allObjects.pop_back();
	}
	EXPECT_EQ(respawnActual, 1u);

	EXPECT_FALSE(isGameLose);//Check that we still don't lose because of having extra life

	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	_allObjects.pop_back();

	EXPECT_EQ(respawnActual, 0u);
	EXPECT_TRUE(isGameLose);//Check that we lose after one death after

}

// Player team lose with broken base
TEST_F(GameStateManagerTest, PlayerTeamLoseWithBrokenBaseAndExtraLife)
{
	bool isGameLose{false};
	auto gameLoseSub = _events->AddListener([&isGameLose](const EnemiesTeamIsWonEvent&) { isGameLose = true; });

	unsigned short respawnEnemyActual{20u};
	unsigned short respawnPlayerOneActual{3u};
	unsigned short respawnPlayerTwoActual{3u};
	auto respawnCountSub = _events->AddListener(
			[&respawnEnemyActual, &respawnPlayerOneActual, &respawnPlayerTwoActual](
			const RespawnCountChangedToEvent& event)
			{
				if (event.objectName == "Enemy")
				{
					respawnEnemyActual = event.respawnCount;
				}
				else if (event.objectName == "Player1")
				{
					respawnPlayerOneActual = event.respawnCount;
				}
				else if (event.objectName == "Player2")
				{
					respawnPlayerTwoActual = event.respawnCount;
				}
			});

	EXPECT_EQ(respawnEnemyActual, 20u);
	EXPECT_EQ(respawnPlayerOneActual, 3u);
	EXPECT_EQ(respawnPlayerTwoActual, 3u);
	constexpr bool skipDelay{true};
	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::OnePlayer});
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	EXPECT_EQ(respawnEnemyActual, 16u);
	EXPECT_EQ(respawnPlayerOneActual, 2u);
	EXPECT_EQ(respawnPlayerTwoActual, 3u);//game mode one player so second should not respawn

	_allObjects.emplace_back(std::make_shared<EagleTile>(ObjRectangle{}, _events, _uuid, GameMode::OnePlayer));
	_allObjects.pop_back();// remove eagle
	EXPECT_EQ(respawnPlayerOneActual, 0u);
	EXPECT_EQ(respawnPlayerTwoActual, 0u);

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
	_events->EmitEvent(Key(std::string{"P1"}), MoveUpEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(respawnPlayerOneActual, 1u);

	_allObjects.pop_back();//remove bonus
	_allObjects.pop_back();//remove player

	EXPECT_FALSE(isGameLose);

	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});

	_allObjects.pop_back();//remove player again (last extra life)

	EXPECT_TRUE(isGameLose);

}
