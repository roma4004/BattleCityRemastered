#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/TankSpawner.h"
#include "components/managers/RespawnManager.h"
#include "enums/Author.h"
#include "enums/Faction.h"
#include "enums/GameMode.h"
#include "enums/InputChannel.h"
#include "enums/TankType.h"
#include "utils/UuidUtils.h"
#include "gtest/gtest.h"
#include <algorithm>
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
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_allObjects.reserve(6u);
		const auto bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, _gameConfig.gameMode, _respawnManager,
								 _tankSpawner);
		_events->EmitEvent(GameResetEvent{});
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
	}

	void TearDown() override {}
};

TEST_F(TankSpawnerTest, DemoPhaseStart)
{
	_gameConfig.gameState = GameState::Demo;
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::CoopWithBot, _respawnManager,
							 _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	EXPECT_EQ(_allObjects.size(), 6u);
}

TEST_F(TankSpawnerTest, OnePlayersGameModeStart)
{
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::OnePlayer, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	EXPECT_EQ(_allObjects.size(), 5u);
}

TEST_F(TankSpawnerTest, TwoPlayersGameModeStart)
{
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::TwoPlayers, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	EXPECT_EQ(_allObjects.size(), 6u);
}

TEST_F(TankSpawnerTest, CoopWithBotGameModeStart)
{
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::CoopWithBot, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	EXPECT_EQ(_allObjects.size(), 6u);
}

TEST_F(TankSpawnerTest, PlayAsHostGameModeStart)
{
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsHost, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	EXPECT_EQ(_allObjects.size(), 6u);// No one set pause, so expected spawn all
}

// A client puts no tank on the field on its own - it waits for the host to say the spawn is done
TEST_F(TankSpawnerTest, PlayAsClientGameModeStart)
{
	std::vector<Uuid> spawning{};
	auto spawnSub = _events->AddListener([&spawning](const TankSpawnEvent& event)
	{
		spawning.emplace_back(event.uuid);
	});

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsClient, _respawnManager, _tankSpawner);
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

//NOTE: the instant-animation wiring is dropped on purpose - the grenade only has something to cancel
//while the spawns are still pending
TEST_F(TankSpawnerTest, GrenadeCancelsEnemiesStillSpawning)
{
	_instantSpawnAnimationSubs.clear();

	std::vector<Uuid> pending{};
	const EventSubscription pendingSub = _events->AddListener(
			[&pending](const AnimationCreateTankSpawnEvent& event) { pending.push_back(event.uuid); });

	//NOTE: the death is all that crosses the wire, so its uuid is the client's only way to match
	std::vector<Uuid> announcedDead{};
	const EventSubscription diedSub = _events->AddListener(
			[&announcedDead](const TankDiedEvent& event) { announcedDead.push_back(event.uuid); });

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::OnePlayer, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});

	ASSERT_FALSE(pending.empty());
	EXPECT_TRUE(_allObjects.empty());

	_events->EmitEvent(Key(Faction::EnemyTeam), BonusGrenadePickupEvent{});

	for (const Uuid& uuid: pending)
	{
		_events->EmitEvent(SpawnAnimationFinishedEvent{.uuid = uuid});
	}

	//the player was not the grenade's target and still arrives; the four enemies never do
	EXPECT_EQ(_allObjects.size(), 1u);

	EXPECT_EQ(announcedDead.size(), 4u);
	for (const Uuid& dead: announcedDead)
	{
		EXPECT_NE(std::ranges::find(pending, dead), pending.end())
				<< "a cancelled spawn was announced under a uuid no client could match";
	}
}

//NOTE: no cancel command exists - a client drops its pending entry on the death itself
TEST_F(TankSpawnerTest, AClientDropsASpawnTheHostCancelled)
{
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsClient, _respawnManager,
							 _tankSpawner);
	_events->EmitEvent(GameResetEvent{});

	const Uuid uuid{UuidUtils::GetRandomUuid()};
	_events->EmitEvent(TankRespawnedEvent{.type = TankType::ENEMY1, .uuid = uuid, .pos = {.x = 0.0, .y = 0.0}});
	_events->EmitEvent(TankDiedEvent{.who = Author::Enemy1, .uuid = uuid, .author = Author::None});
	_events->EmitEvent(TankSpawnCompletedEvent{.uuid = uuid});

	EXPECT_TRUE(_allObjects.empty());
}

//NOTE: a seat keeps its uuid, so a stale entry would land the tank on the cancelled cycle's rect
TEST_F(TankSpawnerTest, AClientSpawnsOnTheLatestRectAfterACancel)
{
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsClient, _respawnManager,
							 _tankSpawner);
	_events->EmitEvent(GameResetEvent{});

	const Uuid uuid{UuidUtils::GetRandomUuid()};
	constexpr FPoint cancelledPos{.x = 0.0, .y = 0.0};
	constexpr FPoint currentPos{.x = 96.0, .y = 0.0};

	_events->EmitEvent(TankRespawnedEvent{.type = TankType::ENEMY1, .uuid = uuid, .pos = cancelledPos});
	_events->EmitEvent(TankDiedEvent{.who = Author::Enemy1, .uuid = uuid, .author = Author::None});
	_events->EmitEvent(TankRespawnedEvent{.type = TankType::ENEMY1, .uuid = uuid, .pos = currentPos});
	_events->EmitEvent(TankSpawnCompletedEvent{.uuid = uuid});

	ASSERT_EQ(_allObjects.size(), 1u);
	EXPECT_EQ(_allObjects.front()->GetRect().x, currentPos.x);
}

TEST_F(TankSpawnerTest, AHostTakesBothSeatsOffTheWire)
{
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsHost, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTankEvent{.type = TankType::PLAYER1, .uuid = UuidUtils::GetRandomUuid()});

	ASSERT_EQ(_allObjects.size(), 1u);
	const auto& playerOne = _allObjects.front();
	const FPoint startPos{playerOne->GetPos()};

	_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = true});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = 1.0 / 60.0});
	EXPECT_EQ(startPos, playerOne->GetPos()) << "the machine running the server drove a seat over its keyboard";

	_events->EmitEvent(Key(InputChannel::RemoteP1), MoveUpEvent{.isPressed = true});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = 1.0 / 60.0});
	EXPECT_NE(startPos, playerOne->GetPos()) << "the first seat never heard the wire";
}
