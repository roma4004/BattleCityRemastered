#include "TestUtils.h"
#include "components/ObstacleSpawner.h"
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
#include <optional>

class TankSpawnerTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::shared_ptr<TankSpawner> _tankSpawner{nullptr};
	std::shared_ptr<RespawnManager> _respawnManager{nullptr};
	std::vector<EventSubscription> _instantSpawnAnimationSubs{};
	GameConfig _gameConfig{};
	std::vector<std::shared_ptr<BaseObj>> _allObjects;
	EventSubscription _spawnQueueSub{};

	void SetUp() override
	{
		_events = std::make_shared<EventSystem>();
		_obstacleSpawner = std::make_unique<ObstacleSpawner>(_events, _gameConfig);
		_spawnQueueSub = TestUtils::WireSpawnQueue(_events, _allObjects);
		_allObjects.reserve(6u);
		const auto bulletPool{std::make_shared<BulletPool>(_events, _allObjects, _gameConfig)};
		TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, _gameConfig.gameMode, _respawnManager,
								 _tankSpawner);
		_events->EmitEvent(GameResetEvent{});
		_instantSpawnAnimationSubs = TestUtils::WireInstantSpawnAnimations(_events);
	}

	void TearDown() override {}

	// a wall across the whole top row except one opening, laid cell by cell the way a map does it
	void WallOffTopRow(const double gapFrom, const double gapTo) const
	{
		const double cell{_gameConfig.gridOffset};
		const auto width{static_cast<double>(_gameConfig.battlefieldSize.x)};
		for (double x{0.0}; x < width; x += cell)
		{
			if (x + cell > gapFrom && x < gapTo)
			{
				continue;
			}

			SpawnObstacle(FPoint{.x = x, .y = 0.0}, ObstacleType::Steel);
		}
	}

	//NOTE: the top row only - players come up at the bottom whatever happens to the enemies' side
	[[nodiscard]] std::size_t CountTanksInTopRow() const
	{
		return static_cast<std::size_t>(std::ranges::count_if(_allObjects, [](const std::shared_ptr<BaseObj>& obj)
		{
			return std::dynamic_pointer_cast<Tank>(obj) != nullptr && obj->GetRect().y == 0.0;
		}));
	}

	std::shared_ptr<BaseObj> SpawnObstacle(const FPoint pos, const ObstacleType type) const
	{
		return TestUtils::SpawnObstacle(_events, _allObjects, pos, type, _gameConfig);
	}
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
	EXPECT_EQ(_allObjects.size(), 6u);
}

// A client puts no tank on the field on its own - it waits for the server to say the spawn is done
TEST_F(TankSpawnerTest, PlayAsClientGameModeStart)
{
	std::vector<Uuid> spawning{};
	auto spawnSub{_events->AddListener([&spawning](const TankSpawnEvent& event)
	{
		spawning.emplace_back(event.uuid);
	})};

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
	const EventSubscription pendingSub{_events->AddListener(
			[&pending](const AnimationCreateTankSpawnEvent& event) { pending.push_back(event.uuid); })};

	//NOTE: the death is all that crosses the wire, so its uuid is the client's only way to match
	std::vector<Uuid> announcedDead{};
	const EventSubscription diedSub{_events->AddListener(
			[&announcedDead](const TankDiedEvent& event) { announcedDead.push_back(event.uuid); })};

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

TEST_F(TankSpawnerTest, AServerBurstCountsTheSpawnDown)
{
	std::vector<AnimationCreateTankSpawnEvent> bursts{};
	const EventSubscription burstSub{_events->AddListener(
			[&bursts](const AnimationCreateTankSpawnEvent& event) { bursts.push_back(event); })};

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsHost, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTankEvent{.type = TankType::PLAYER1, .uuid = UuidUtils::GetRandomUuid()});

	ASSERT_EQ(bursts.size(), 1u);
	EXPECT_FALSE(bursts.front().isEndless);
}

//NOTE: the server's loop may run slower than ours - a burst of our own would end before the tank lands
TEST_F(TankSpawnerTest, AClientBurstWaitsForTheServer)
{
	std::optional<AnimationCreateTankSpawnEvent> burst{};
	const EventSubscription burstSub{_events->AddListener(
			[&burst](const AnimationCreateTankSpawnEvent& event) { burst = event; })};

	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsClient, _respawnManager,
							 _tankSpawner);
	_events->EmitEvent(GameResetEvent{});

	const Uuid uuid{UuidUtils::GetRandomUuid()};
	_events->EmitEvent(TankRespawnedEvent{.type = TankType::ENEMY1, .uuid = uuid, .pos = {.x = 0.0, .y = 0.0}});

	ASSERT_TRUE(burst.has_value());
	EXPECT_TRUE(burst->isEndless);
	EXPECT_TRUE(_allObjects.empty());

	_events->EmitEvent(TankSpawnCompletedEvent{.uuid = uuid});

	EXPECT_EQ(_allObjects.size(), 1u);
}

//NOTE: no cancel command exists - a client drops its pending entry on the death itself
TEST_F(TankSpawnerTest, AClientDropsASpawnTheServerCancelled)
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

TEST_F(TankSpawnerTest, AServerTakesBothSeatsOffTheWire)
{
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsHost, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTankEvent{.type = TankType::PLAYER1, .uuid = UuidUtils::GetRandomUuid()});

	ASSERT_EQ(_allObjects.size(), 1u);
	const auto& playerOne{_allObjects.front()};
	const FPoint startPos{playerOne->GetPos()};

	_events->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = true});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = 1.0 / 60.0});
	EXPECT_EQ(startPos, playerOne->GetPos()) << "the machine running the server drove a seat over its keyboard";

	_events->EmitEvent(Key(InputChannel::RemoteP1), MoveUpEvent{.isPressed = true});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = 1.0 / 60.0});
	EXPECT_NE(startPos, playerOne->GetPos()) << "the first seat never heard the wire";
}

// the only opening in the top row sits off both the tank and half-tank step - a search by strides alone
// would miss it and the enemy would never appear
TEST_F(TankSpawnerTest, AnEnemyFindsAnOpeningThatIsOffTheCoarseSteps)
{
	//NOTE: exactly one tank wide, set one cell off the tank-sized stride
	const double tankSize{_gameConfig.tankSize};
	const double gapFrom{tankSize * 2.0 + _gameConfig.gridOffset};
	WallOffTopRow(gapFrom, gapFrom + tankSize);

	const std::size_t before{CountTanksInTopRow()};
	_events->EmitEvent(RespawnTanksEvent{});

	EXPECT_GT(CountTanksInTopRow(), before) << "no enemy took the one opening there was";
}
