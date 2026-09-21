#include "TestUtils.h"
#include "components/ObstacleSpawner.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/TankPool.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/TimingEvents.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "components/managers/GameStateManager.h"
#include "entities/obstacles/EagleTile.h"
#include "entities/pawns/Tank.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/InputChannel.h"
#include "utils/UuidUtils.h"
#include "gtest/gtest.h"
#include <iostream>
#include <memory>

// each case plays a one-player match out by hand - kill everything in _allObjects, ask for a respawn,
// repeat - and watches the life counts until GameFinishedEvent says Won or Over
class GameStateManagerTest : public testing::Test// NOLINT(clang-diagnostic-padded)
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<TankPool> _tankPool{nullptr};
	std::shared_ptr<BonusSpawner> _bonusSpawner{nullptr};
	std::shared_ptr<GameStateManager> _stateManager{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<RespawnManager> _respawnManager{nullptr};
	std::vector<EventSubscription> _instantSpawnAnimationSubs{};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	double _deltaTimeOneFrame{1.0 / 60.0};
	Uuid _uuid{};
	double _tankSize{};
	double _gridSize{};
	unsigned short _tankHealth{100u};
	GameMode _gameMode{GameMode::OnePlayer};
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_obstacleSpawner = std::make_unique<ObstacleSpawner>(_events, _gameConfig);
		_gameConfig.gameMode = _gameMode;
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_tankPool = std::make_shared<TankPool>(_events, _allObjects, _gameConfig, _bulletPool);
		_bonusSpawner = std::make_unique<BonusSpawner>(_events, _allObjects, _gameConfig);
		_stateManager = std::make_shared<GameStateManager>(_events);
		TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, _gameConfig.gameMode, _respawnManager,
								 _tankSpawner);
		_events->EmitEvent(GameResetEvent{});
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
		_gridSize = _gameConfig.gridOffset;
		_tankSize = _gameConfig.tankSize;

		_allObjects.reserve(4u);
	}

	void TearDown() override {}

	std::shared_ptr<Tank> CreateBot(const FPoint pos, const Author author, const Direction dir)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};
		auto bot{TestUtils::CreateBot(rect, _tankHealth, author, _allObjects, _events, dir, _tankPool, _gameConfig)};

		return bot;
	}

	std::shared_ptr<Tank> CreatePlayer(const FPoint pos, const Author author, const Direction dir)
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};
		auto player{TestUtils::CreatePlayer(rect, _tankHealth, author, _allObjects, _events, dir, _tankPool,
											_gameConfig)};

		return player;
	}

	std::shared_ptr<BaseObj> SpawnObstacle(const ObjRectangle rect, const ObstacleType type) const
	{
		return TestUtils::SpawnObstacle(_events, _allObjects, rect, type);
	}
};

// nothing but the plain course of a match: five waves of four enemies each empty the pool of 20
TEST_F(GameStateManagerTest, PlayerTeamWon)
{
	bool isGameWon{};

	std::vector<std::pair<unsigned short, Uuid>> howManySpawnCounters;
	howManySpawnCounters.reserve(4u);
	auto spawnCounterSub = _events->AddListener([&howManySpawnCounters](const TankSpawnEvent& tankSpawnEvent)
	{
		const auto& uuid{tankSpawnEvent.uuid};
		const auto it{std::ranges::find_if(howManySpawnCounters,
										   [&uuid](const std::pair<unsigned short, Uuid>& p)
										   {
											   return p.second == uuid;
										   })};

		if (it != howManySpawnCounters.end())
		{
			it->first++;
		}
		else
		{
			howManySpawnCounters.emplace_back(1u, uuid);
		}
	});

	std::vector<std::pair<unsigned short, Uuid>> howManyDiedCounters;
	howManyDiedCounters.reserve(4u);
	auto diedCounterSub = _events->AddListener([&howManyDiedCounters](const TankDiedEvent& tankDiedEvent)
	{
		const auto& uuid{tankDiedEvent.uuid};
		const auto it{std::ranges::find_if(howManyDiedCounters,
										   [&uuid](const std::pair<unsigned short, Uuid>& p)
										   {
											   return p.second == uuid;
										   })};

		if (it != howManyDiedCounters.end())
		{
			it->first++;
		}
		else
		{
			howManyDiedCounters.emplace_back(1u, uuid);
		}
	});

	auto gameWonSub{_events->AddListener([&isGameWon](const GameFinishedEvent& event)
	{
		isGameWon = event.state == GameState::Won;
	})};

	unsigned short respawnEnemyActual{20u};
	unsigned short respawnPlayerOneActual{3u};
	unsigned short respawnPlayerTwoActual{3u};
	auto respawnCountSub = _events->AddListener(
			[&respawnEnemyActual, &respawnPlayerOneActual, &respawnPlayerTwoActual](
			const RespawnCountChangedToEvent& event)
			{
				if (event.group == RespawnGroup::ENEMY_ALL)
				{
					respawnEnemyActual = event.respawnCount;
				}
				else if (event.group == RespawnGroup::PLAYER1)
				{
					respawnPlayerOneActual = event.respawnCount;
				}
				else if (event.group == RespawnGroup::PLAYER2)
				{
					respawnPlayerTwoActual = event.respawnCount;
				}
			});

	EXPECT_FALSE(isGameWon);
	for (unsigned short i = 0u; i < 5u; ++i)
	{
		for (const auto& object: _allObjects)
		{
			_events->EmitEvent(TankDiedEvent{.uuid = object->GetUuid()});
		}
		_allObjects.clear();
		EXPECT_EQ(_allObjects.size(), 0u);

		EXPECT_EQ(respawnEnemyActual, 20u - i * 4u);
		_events->EmitEvent(RespawnTanksEvent{});
		std::cout << "End of respawn round" << (i + 1u) << '\n';
	}

	for (const auto& object: _allObjects)
	{
		_events->EmitEvent(TankDiedEvent{.uuid = object->GetUuid()});
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

// an enemy takes the extra-life tank first, so the pool holds 21 and the win waits for that one
// leftover enemy
TEST_F(GameStateManagerTest, PlayerTeamWonWithEnemyExtraLife)
{
	bool isGameWon{};

	std::vector<std::pair<unsigned short, Uuid>> howManySpawnCounters;
	howManySpawnCounters.reserve(4u);
	auto spawnCounterSub = _events->AddListener([&howManySpawnCounters](const TankSpawnEvent& tankSpawnEvent)
	{
		const auto& uuid{tankSpawnEvent.uuid};
		const auto it{std::ranges::find_if(howManySpawnCounters,
										   [&uuid](const std::pair<unsigned short, Uuid>& p)
										   {
											   return p.second == uuid;
										   })};

		if (it != howManySpawnCounters.end())
		{
			it->first++;
		}
		else
		{
			howManySpawnCounters.emplace_back(1u, uuid);
		}
	});

	std::vector<std::pair<unsigned short, Uuid>> howManyDiedCounters;
	howManyDiedCounters.reserve(4u);
	auto diedCounterSub = _events->AddListener([&howManyDiedCounters](const TankDiedEvent& tankDiedEvent)
	{
		const auto& uuid{tankDiedEvent.uuid};
		const auto it{std::ranges::find_if(howManyDiedCounters,
										   [&uuid](const std::pair<unsigned short, Uuid>& p)
										   {
											   return p.second == uuid;
										   })};

		if (it != howManyDiedCounters.end())
		{
			it->first++;
		}
		else
		{
			howManyDiedCounters.emplace_back(1u, uuid);
		}
	});

	auto gameWonSub{_events->AddListener([&isGameWon](const GameFinishedEvent& event)
	{
		isGameWon = event.state == GameState::Won;
	})};

	unsigned short respawnEnemyActual{20u};
	unsigned short respawnPlayerOneActual{3u};
	unsigned short respawnPlayerTwoActual{3u};
	auto respawnCountSub = _events->AddListener(
			[&respawnEnemyActual, &respawnPlayerOneActual, &respawnPlayerTwoActual](
			const RespawnCountChangedToEvent& event)
			{
				if (event.group == RespawnGroup::ENEMY_ALL)
				{
					respawnEnemyActual = event.respawnCount;
				}
				else if (event.group == RespawnGroup::PLAYER1)
				{
					respawnPlayerOneActual = event.respawnCount;
				}
				else if (event.group == RespawnGroup::PLAYER2)
				{
					respawnPlayerTwoActual = event.respawnCount;
				}
			});

	const auto enemyBot{CreateBot({.x = _tankSize * 3.0, .y = _tankSize * 3.0}, Author::Enemy1, Direction::DOWN)};

	// Spawn bonus extra life
	_bonusSpawner->SpawnBonus(
			{.x = _tankSize * 3.0, .y = _tankSize * 3.0 + _tankSize + 1.0, .w = _tankSize, .h = _tankSize},
			BonusType::Tank);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(respawnEnemyActual, 21u);

	for (unsigned short i = 0u; i < 4u; ++i)
	{
		for (const auto& object: _allObjects)
		{
			_events->EmitEvent(TankDiedEvent{.uuid = object->GetUuid()});
		}
		_allObjects.clear();
		EXPECT_EQ(_allObjects.size(), 0u);

		EXPECT_EQ(respawnEnemyActual, 21u - i * 4u);
		_events->EmitEvent(RespawnTanksEvent{});
		std::cout << "End of respawn round" << (i + 1u) << " with remain enemy respawn" << respawnEnemyActual << '\n';
	}

	for (const auto& object: _allObjects)
	{
		_events->EmitEvent(TankDiedEvent{.uuid = object->GetUuid()});
	}
	_allObjects.clear();
	EXPECT_EQ(_allObjects.size(), 0u);
	EXPECT_EQ(respawnEnemyActual, 5u);

	EXPECT_FALSE(isGameWon);

	std::cout << "spawn extra life tank" << '\n';
	_events->EmitEvent(RespawnTanksEvent{});//spawn 4 enemies
	EXPECT_EQ(_allObjects.size(), 4u);
	_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
	_allObjects.pop_back();//one enemy tank died
	_events->EmitEvent(RespawnTanksEvent{});//spawn use extra life
	EXPECT_EQ(_allObjects.size(), 4u);
	for (const auto& object: _allObjects)
	{
		_events->EmitEvent(TankDiedEvent{.uuid = object->GetUuid()});
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

// the eagle falls with the player still alive: every remaining life is taken away at once, and the
// death that follows ends the match
TEST_F(GameStateManagerTest, PlayerTeamLoseWithBrokenBase)
{
	bool isGameLose{};
	auto gameLoseSub{_events->AddListener([&isGameLose](const GameFinishedEvent& event)
	{
		isGameLose = event.state == GameState::Over;
	})};

	unsigned short respawnActual{3u};
	auto respawnCountSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		respawnActual = event.respawnCount;
	})};

	EXPECT_EQ(respawnActual, 3u);
	_events->EmitEvent(RespawnTanksEvent{});
	SpawnObstacle(ObjRectangle{}, ObstacleType::Eagle);
	EXPECT_EQ(respawnActual, 2u);

	EXPECT_FALSE(isGameLose);

	_events->EmitEvent(PlayersBaseFinishedEvent{});
	_allObjects.pop_back();// the base fell
	EXPECT_EQ(respawnActual, 0u);
	_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
	_allObjects.pop_back();// the player died

	EXPECT_TRUE(isGameLose);

}

// the base stands - the player simply dies three times
TEST_F(GameStateManagerTest, PlayerTeamLoseWithThreeDeath)
{
	bool isGameLose{};
	auto gameLoseSub{_events->AddListener([&isGameLose](const GameFinishedEvent& event)
	{
		isGameLose = event.state == GameState::Over;
	})};

	unsigned short respawnActual{3u};
	auto respawnCountSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::PLAYER1)
		{
			respawnActual = event.respawnCount;
		}
	})};

	EXPECT_FALSE(isGameLose);

	EXPECT_EQ(respawnActual, 3u);
	for (unsigned short i = 0u; i < 3u; ++i)
	{
		_events->EmitEvent(RespawnTanksEvent{});
		_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
		_allObjects.pop_back();
	}

	EXPECT_EQ(respawnActual, 0u);
	EXPECT_TRUE(isGameLose);

}

// the player takes the extra-life tank first, so it takes four deaths instead of three
TEST_F(GameStateManagerTest, PlayerTeamLoseWithExtraLifeDeath)
{
	const auto player{CreatePlayer({.x = 0.0, .y = 0.0}, Author::Player1, Direction::UP)};

	bool isGameLose{};
	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveDownEvent{.isPressed = isPressed});

	auto gameLoseSub{_events->AddListener([&isGameLose](const GameFinishedEvent& event)
	{
		isGameLose = event.state == GameState::Over;
	})};

	unsigned short respawnActual{3u};
	auto respawnCountSub{_events->AddListener([&respawnActual](const RespawnCountChangedToEvent& event)
	{
		if (event.group == RespawnGroup::PLAYER1)
		{
			respawnActual = event.respawnCount;
		}
	})};

	// Spawn bonus extra life
	_bonusSpawner->SpawnBonus({.x = 0.0, .y = _tankSize + 1.0, .w = _tankSize, .h = _tankSize}, BonusType::Tank);

	EXPECT_EQ(respawnActual, 3u);

	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(respawnActual, 4u);
	for (unsigned short i = 0u; i < 3u; ++i)
	{
		_events->EmitEvent(RespawnTanksEvent{});
		_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
		_allObjects.pop_back();
	}
	EXPECT_EQ(respawnActual, 1u);

	EXPECT_FALSE(isGameLose);

	_events->EmitEvent(RespawnTanksEvent{});
	_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
	_allObjects.pop_back();

	EXPECT_EQ(respawnActual, 0u);
	EXPECT_TRUE(isGameLose);

}

// the base falls (lives to zero), and only then the player takes an extra life: it buys exactly one
// more death, not a return to three
TEST_F(GameStateManagerTest, PlayerTeamLoseWithBrokenBaseAndExtraLife)
{
	bool isGameLose{};
	auto gameLoseSub{_events->AddListener([&isGameLose](const GameFinishedEvent& event)
	{
		isGameLose = event.state == GameState::Over;
	})};

	unsigned short respawnEnemyActual{20u};
	unsigned short respawnPlayerOneActual{3u};
	unsigned short respawnPlayerTwoActual{3u};
	auto respawnCountSub = _events->AddListener(
			[&respawnEnemyActual, &respawnPlayerOneActual, &respawnPlayerTwoActual](
			const RespawnCountChangedToEvent& event)
			{
				if (event.group == RespawnGroup::ENEMY_ALL)
				{
					respawnEnemyActual = event.respawnCount;
				}
				else if (event.group == RespawnGroup::PLAYER1)
				{
					respawnPlayerOneActual = event.respawnCount;
				}
				else if (event.group == RespawnGroup::PLAYER2)
				{
					respawnPlayerTwoActual = event.respawnCount;
				}
			});

	EXPECT_EQ(respawnEnemyActual, 20u);
	EXPECT_EQ(respawnPlayerOneActual, 3u);
	EXPECT_EQ(respawnPlayerTwoActual, 3u);
	_events->EmitEvent(RespawnTanksEvent{});
	EXPECT_EQ(respawnEnemyActual, 16u);
	EXPECT_EQ(respawnPlayerOneActual, 2u);
	EXPECT_EQ(respawnPlayerTwoActual, 3u);//game mode one player so second should not respawn

	SpawnObstacle(ObjRectangle{}, ObstacleType::Eagle);
	_events->EmitEvent(PlayersBaseFinishedEvent{});
	_allObjects.pop_back();// the base fell
	EXPECT_EQ(respawnPlayerOneActual, 0u);
	EXPECT_EQ(respawnPlayerTwoActual, 0u);

	if (const auto player{dynamic_cast<Tank*>(_allObjects.back().get())})
	{
		auto [x, y] = player->GetPos();

		// Spawn bonus extra life near player
		_bonusSpawner->SpawnBonus({.x = x, .y = y - _tankSize + 1.0, .w = _tankSize, .h = _tankSize},
								  BonusType::Tank);
	}
	else
	{
		EXPECT_FALSE(true);
	}

	constexpr bool isPressed{true};
	_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = isPressed});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = _deltaTimeOneFrame});

	EXPECT_EQ(respawnPlayerOneActual, 1u);

	_allObjects.pop_back();//the bonus is gone, nobody died
	_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
	_allObjects.pop_back();//the player died

	EXPECT_FALSE(isGameLose);

	_events->EmitEvent(RespawnTanksEvent{});

	_events->EmitEvent(TankDiedEvent{.uuid = _allObjects.back()->GetUuid()});
	_allObjects.pop_back();//the player died again (last extra life)

	EXPECT_TRUE(isGameLose);

}
