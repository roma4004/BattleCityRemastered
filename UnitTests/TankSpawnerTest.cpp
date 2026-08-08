#include "TestUtils.h"
#include "application/GameConfig.h"
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
	GameConfig _gameConfig{"", true};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		TestUtils::WireSpawnQueue(_events, &_allObjects);
		_allObjects.reserve(6u);
		const auto bulletPool = std::make_shared<BulletPool>(_events, &_allObjects, _gameConfig);
		_respawnManager = std::make_shared<RespawnManager>(_events);
		_tankSpawner = std::make_shared<TankSpawner>(_gameConfig, &_allObjects, _events);
		_spawnDelayManager = std::make_shared<DelayedSpawnManager>(_events);
	}

	void TearDown() override
	{
		_events->RemoveListener<AddToSpawnQueueEvent>("TestSpawnQueue");
	}
};

TEST_F(TankSpawnerTest, DemoGameModeStart)
{
	constexpr bool skipDelay{true};
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	EXPECT_EQ(_allObjects.size(), 0u);

	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::Demo});
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	EXPECT_EQ(_allObjects.size(), 6u);
}

TEST_F(TankSpawnerTest, OnePlayersGameModeStart)
{
	constexpr bool skipDelay{true};
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	EXPECT_EQ(_allObjects.size(), 0u);

	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::OnePlayer});
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	EXPECT_EQ(_allObjects.size(), 5u);
}

TEST_F(TankSpawnerTest, TwoPlayersGameModeStart)
{
	constexpr bool skipDelay{true};
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	EXPECT_EQ(_allObjects.size(), 0u);

	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::TwoPlayers});
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	EXPECT_EQ(_allObjects.size(), 6u);
}

TEST_F(TankSpawnerTest, CoopWithBotGameModeStart)
{
	constexpr bool skipDelay{true};
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	EXPECT_EQ(_allObjects.size(), 0u);

	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::CoopWithBot});
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	EXPECT_EQ(_allObjects.size(), 6u);
}

TEST_F(TankSpawnerTest, PlayAsHostGameModeStart)
{
	constexpr bool skipDelay{true};
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	EXPECT_EQ(_allObjects.size(), 0u);

	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::PlayAsHost});
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	EXPECT_EQ(_allObjects.size(), 6u);// No one set pause, so expected spawn all
}

TEST_F(TankSpawnerTest, PlayAsClientGameModeStart)
{
	constexpr bool skipDelay{true};
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	EXPECT_EQ(_allObjects.size(), 0u);

	_events->EmitEvent(GameModeChangedToEvent{.mode = GameMode::PlayAsClient});
	_events->EmitEvent(RespawnTanksEvent{.skipDelay = skipDelay});
	EXPECT_EQ(_allObjects.size(), 6u);// No one set pause, so expected spawn all
}
