#include "TestUtils.h"
#include "application/GameConfig.h"
#include "application/ProjectConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/TankSpawner.h"
#include "components/managers/DelayedSpawnManager.h"
#include "components/managers/RespawnManager.h"
#include "enums/GameMode.h"
#include "gtest/gtest.h"
#include <memory>

class TankSpawnerTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<RespawnManager> _respawnManager{nullptr};
	std::shared_ptr<DelayedSpawnManager> _spawnDelayManager{nullptr};
	ProjectConfig _projectConfig{"", true};
	GameConfig _gameConfig{_projectConfig};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, &_allObjects);
		_allObjects.reserve(6u);
		const auto bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _gameConfig);
		TestUtils::ApplyGameMode(_events, &_allObjects, _gameConfig, _gameConfig.gameMode, _respawnManager,
								 _tankSpawner);
		_spawnDelayManager = std::make_shared<DelayedSpawnManager>(_events, _gameConfig);
	}

	void TearDown() override
	{
	}
};

TEST_F(TankSpawnerTest, DemoGameModeStart)
{
	constexpr bool skipDelay{true};
	TestUtils::ApplyGameMode(_events, &_allObjects, _gameConfig, GameMode::Demo, _respawnManager, _tankSpawner);
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	EXPECT_EQ(_allObjects.size(), 6u);
}

TEST_F(TankSpawnerTest, OnePlayersGameModeStart)
{
	constexpr bool skipDelay{true};
	TestUtils::ApplyGameMode(_events, &_allObjects, _gameConfig, GameMode::OnePlayer, _respawnManager, _tankSpawner);
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	EXPECT_EQ(_allObjects.size(), 5u);
}

TEST_F(TankSpawnerTest, TwoPlayersGameModeStart)
{
	constexpr bool skipDelay{true};
	TestUtils::ApplyGameMode(_events, &_allObjects, _gameConfig, GameMode::TwoPlayers, _respawnManager, _tankSpawner);
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	EXPECT_EQ(_allObjects.size(), 6u);
}

TEST_F(TankSpawnerTest, CoopWithBotGameModeStart)
{
	constexpr bool skipDelay{true};
	TestUtils::ApplyGameMode(_events, &_allObjects, _gameConfig, GameMode::CoopWithBot, _respawnManager, _tankSpawner);
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	EXPECT_EQ(_allObjects.size(), 6u);
}

TEST_F(TankSpawnerTest, PlayAsHostGameModeStart)
{
	constexpr bool skipDelay{true};
	TestUtils::ApplyGameMode(_events, &_allObjects, _gameConfig, GameMode::PlayAsHost, _respawnManager, _tankSpawner);
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	EXPECT_EQ(_allObjects.size(), 6u);// No one set pause, so expected spawn all
}

TEST_F(TankSpawnerTest, PlayAsClientGameModeStart)
{
	constexpr bool skipDelay{true};
	TestUtils::ApplyGameMode(_events, &_allObjects, _gameConfig, GameMode::PlayAsClient, _respawnManager, _tankSpawner);
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	EXPECT_EQ(_allObjects.size(), 6u);// No one set pause, so expected spawn all
}
