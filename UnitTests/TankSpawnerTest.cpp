#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/TankSpawner.h"
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
	std::vector<EventSubscription> _instantSpawnAnimationSubs{};
	GameConfig _gameConfig{};
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
		_events->EmitEvent(GameResetEvent{});
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
	}

	void TearDown() override
	{
	}
};

TEST_F(TankSpawnerTest, DemoGameModeStart)
{
	TestUtils::ApplyGameMode(_events, &_allObjects, _gameConfig, GameMode::Demo, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	EXPECT_EQ(_allObjects.size(), 6u);
}

TEST_F(TankSpawnerTest, OnePlayersGameModeStart)
{
	TestUtils::ApplyGameMode(_events, &_allObjects, _gameConfig, GameMode::OnePlayer, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	EXPECT_EQ(_allObjects.size(), 5u);
}

TEST_F(TankSpawnerTest, TwoPlayersGameModeStart)
{
	TestUtils::ApplyGameMode(_events, &_allObjects, _gameConfig, GameMode::TwoPlayers, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	EXPECT_EQ(_allObjects.size(), 6u);
}

TEST_F(TankSpawnerTest, CoopWithBotGameModeStart)
{
	TestUtils::ApplyGameMode(_events, &_allObjects, _gameConfig, GameMode::CoopWithBot, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	EXPECT_EQ(_allObjects.size(), 6u);
}

TEST_F(TankSpawnerTest, PlayAsHostGameModeStart)
{
	TestUtils::ApplyGameMode(_events, &_allObjects, _gameConfig, GameMode::PlayAsHost, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	EXPECT_EQ(_allObjects.size(), 6u);// No one set pause, so expected spawn all
}

//Check that a client puts no tank on the field on its own - it waits for the host to say the spawn is done
TEST_F(TankSpawnerTest, PlayAsClientGameModeStart)
{
	std::vector<Uuid> spawning{};
	auto spawnSub = _events->AddListener([&spawning](const TankSpawnEvent& event)
	{
		spawning.emplace_back(event.uuid);
	});

	TestUtils::ApplyGameMode(_events, &_allObjects, _gameConfig, GameMode::PlayAsClient, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});

	EXPECT_EQ(spawning.size(), 6u);
	EXPECT_TRUE(_allObjects.empty());

	for (const Uuid& uuid: spawning)
	{
		_events->EmitEvent(TankSpawnCompletedEvent{.uuid = uuid});
	}

	EXPECT_EQ(_allObjects.size(), 6u);
}
