#include "utils/MathUtils.h"
#include "TestUtils.h"
#include "components/ObstacleSpawner.h"
#include "application/GameConfig.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/TankPool.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/events/TimingEvents.h"
#include "components/TankSpawner.h"
#include "components/WorldSnapshot.h"
#include "components/managers/RespawnManager.h"
#include "entities/BaseObj.h"
#include "enums/Author.h"
#include "enums/Direction.h"
#include "enums/Faction.h"
#include "enums/GameMode.h"
#include "enums/InputChannel.h"
#include "enums/MatchRules.h"
#include "enums/PlayerSlot.h"
#include "enums/TankModel.h"
#include "enums/TankType.h"
#include "utils/ColliderUtils.h"
#include "utils/UuidUtils.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <array>
#include <memory>
#include <optional>
#include <string_view>

class TankSpawnerTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::unique_ptr<ObstacleSpawner> _obstacleSpawner{nullptr};
	std::shared_ptr<BulletPool> _bulletPool{nullptr};
	std::shared_ptr<TankPool> _tankPool{nullptr};
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
		_bulletPool = std::make_shared<BulletPool>(_events, _allObjects, _gameConfig);
		_tankPool = std::make_shared<TankPool>(_events, _allObjects, _gameConfig, _bulletPool);
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

	// the same row planted instead of walled - a tank drives over bush, so it may also be born on it
	void PlantTopRow(const ObstacleType type) const
	{
		const double cell{_gameConfig.gridOffset};
		const auto width{static_cast<double>(_gameConfig.battlefieldSize.x)};
		for (double x{0.0}; x < width; x += cell)
		{
			SpawnObstacle(FPoint{.x = x, .y = 0.0}, type);
		}
	}

	//NOTE: the top row only - players come up at the bottom whatever happens to the enemies' side
	[[nodiscard]] std::size_t CountTanksInTopRow() const
	{
		return static_cast<std::size_t>(std::ranges::count_if(_allObjects, [](const std::shared_ptr<BaseObj>& obj)
		{
			return std::dynamic_pointer_cast<Tank>(obj) != nullptr && MathUtils::AreEqualAbsolute(obj->GetRect().y, 0.0);
		}));
	}

	std::shared_ptr<BaseObj> SpawnObstacle(const FPoint pos, const ObstacleType type) const
	{
		return TestUtils::SpawnObstacle(_events, _allObjects, pos, type, _gameConfig);
	}

	std::shared_ptr<Tank> CreatePlayer(const FPoint pos, const Direction dir) const
	{
		const ObjRectangle rect{.x = pos.x, .y = pos.y, .w = _gameConfig.tankSize, .h = _gameConfig.tankSize};

		return TestUtils::CreatePlayer(rect, _gameConfig.tankHealth, Author::Player1, _allObjects, _events, dir,
									   _tankPool, _gameConfig);
	}
};

// in the demo nobody sits down, so every seat goes to a bot and the field fills up
TEST_F(TankSpawnerTest, DemoPhaseStart)
{
	_gameConfig.gameState = GameState::Demo;
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::CoopWithBot, _respawnManager,
							 _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	EXPECT_EQ(_allObjects.size(), 6u);
}

// one seat taken and four bots
TEST_F(TankSpawnerTest, OnePlayersGameModeStart)
{
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::OnePlayer, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	EXPECT_EQ(_allObjects.size(), 5u);
}

// both seats taken
TEST_F(TankSpawnerTest, TwoPlayersGameModeStart)
{
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::TwoPlayers, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	EXPECT_EQ(_allObjects.size(), 6u);
}

// one player and a bot on its side
TEST_F(TankSpawnerTest, CoopWithBotGameModeStart)
{
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::CoopWithBot, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	EXPECT_EQ(_allObjects.size(), 6u);
}

// and a network match seats the same six
TEST_F(TankSpawnerTest, PlayAsHostGameModeStart)
{
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsHost, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});
	EXPECT_EQ(_allObjects.size(), 6u);
}

// a four-seat host spawns four players, two on each side of the fortress
TEST_F(TankSpawnerTest, AFourSeatHostMatchSeatsFourPlayers)
{
	_gameConfig.networkSeats = 4u;
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsHost, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});

	EXPECT_EQ(_allObjects.size(), 8u);
}

// a host told to play free-for-all sets every tank against every other and keeps the bots it picked on the field
TEST_F(TankSpawnerTest, AFreeForAllHostPutsEveryTankOnItsOwn)
{
	_gameConfig.networkSeats = 4u;
	_gameConfig.networkRules = MatchRules::FreeForAll;
	_gameConfig.simultaneousEnemies = 2u;
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsHost, _respawnManager, _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});

	EXPECT_EQ(_allObjects.size(), 6u);
	EXPECT_TRUE(std::ranges::all_of(_allObjects, [](const std::shared_ptr<BaseObj>& tank)
	{
		return tank->GetFaction() == Faction::Solo;
	}));
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

// an opening one tank wide takes exactly one burst, so the next spawn has to look elsewhere
TEST_F(TankSpawnerTest, ASpawnDoesNotTakeTheSquareOfABurstAlreadyRunning)
{
	_instantSpawnAnimationSubs.clear();

	const double tankSize{_gameConfig.tankSize};
	WallOffTopRow(tankSize * 2.0, tankSize * 3.0);

	std::vector<AnimationCreateTankSpawnEvent> bursts{};
	const EventSubscription burstSub{_events->AddListener(
			[&bursts](const AnimationCreateTankSpawnEvent& event) { bursts.push_back(event); })};

	_events->EmitEvent(RespawnTanksEvent{});

	const auto inTheOpening{std::ranges::count_if(bursts, [](const AnimationCreateTankSpawnEvent& burst)
	{
		return MathUtils::AreEqualAbsolute(burst.rect.y, 0.0);
	})};

	EXPECT_EQ(inTheOpening, 1) << "two bursts were started in one square";
}

TEST_F(TankSpawnerTest, ASpawnSquareIsShovedAlongAndGoesBackWhenItsOwnIsFree)
{
	_instantSpawnAnimationSubs.clear();

	std::optional<ObjRectangle> square{};
	std::vector<EventSubscription> burstSubs{};
	burstSubs.push_back(_events->AddListener([&square](const AnimationCreateTankSpawnEvent& event)
	{
		square = event.rect;
	}));
	burstSubs.push_back(_events->AddListener([&square](const AnimationMoveTankSpawnEvent& event)
	{
		square = event.rect;
	}));

	_events->EmitEvent(RespawnTankEvent{.type = TankType::ENEMY1, .uuid = UuidUtils::GetRandomUuid()});
	ASSERT_TRUE(square.has_value());
	const ObjRectangle home{*square};

	// half a hull into the square, driving along the row
	const auto pusher{CreatePlayer(FPoint{.x = home.x + _gameConfig.tankSize / 2.0, .y = home.y}, Direction::RIGHT)};
	_events->EmitEvent(PostTickUpdateEvent{});

	EXPECT_GT(square->x, home.x) << "the square stayed under the hull that drove into it";
	EXPECT_FALSE(ColliderUtils::IsCollide(*square, pusher->GetRect())) << "shoved, and still under the hull";

	pusher->SetPos(FPoint{.x = home.x, .y = home.y + _gameConfig.tankSize * 3.0});
	_events->EmitEvent(PostTickUpdateEvent{});

	EXPECT_DOUBLE_EQ(square->x, home.x) << "the spawn point came free and the burst did not come back";
	EXPECT_DOUBLE_EQ(square->y, home.y);
}

TEST_F(TankSpawnerTest, ASpawnLandsWhereItsSquareWasShovedTo)
{
	_instantSpawnAnimationSubs.clear();

	std::optional<ObjRectangle> square{};
	std::vector<EventSubscription> burstSubs{};
	burstSubs.push_back(_events->AddListener([&square](const AnimationCreateTankSpawnEvent& event)
	{
		square = event.rect;
	}));
	burstSubs.push_back(_events->AddListener([&square](const AnimationMoveTankSpawnEvent& event)
	{
		square = event.rect;
	}));

	const Uuid uuid{UuidUtils::GetRandomUuid()};
	_events->EmitEvent(RespawnTankEvent{.type = TankType::ENEMY1, .uuid = uuid});
	ASSERT_TRUE(square.has_value());
	const ObjRectangle home{*square};

	CreatePlayer(FPoint{.x = home.x, .y = home.y}, Direction::RIGHT);
	_events->EmitEvent(PostTickUpdateEvent{});
	ASSERT_GT(square->x, home.x);

	_events->EmitEvent(SpawnAnimationFinishedEvent{.uuid = uuid});

	const auto landed{std::ranges::find_if(_allObjects, [uuid](const std::shared_ptr<BaseObj>& obj)
	{
		return obj->GetUuid() == uuid;
	})};
	ASSERT_NE(landed, _allObjects.end());

	EXPECT_DOUBLE_EQ((*landed)->GetRect().x, square->x) << "it did not come up where its burst was playing";
}

// a client is told where the host shoved the burst, because it is also where the host will put the tank
TEST_F(TankSpawnerTest, AClientMovesItsBurstWhereTheHostShovedIt)
{
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsClient, _respawnManager,
							 _tankSpawner);
	_events->EmitEvent(GameResetEvent{});

	std::optional<ObjRectangle> square{};
	const EventSubscription moveSub{_events->AddListener([&square](const AnimationMoveTankSpawnEvent& event)
	{
		square = event.rect;
	})};

	const Uuid uuid{UuidUtils::GetRandomUuid()};
	constexpr FPoint shovedTo{.x = 96.0, .y = 0.0};
	_events->EmitEvent(TankRespawnedEvent{.type = TankType::ENEMY1,
										  .model = TankModel::Basic,
										  .uuid = uuid,
										  .pos = {.x = 0.0, .y = 0.0}});
	_events->EmitEvent(TankSpawnMovedEvent{.uuid = uuid, .pos = shovedTo});
	ASSERT_TRUE(square.has_value());
	EXPECT_DOUBLE_EQ(square->x, shovedTo.x);

	_events->EmitEvent(TankSpawnCompletedEvent{.uuid = uuid});

	ASSERT_EQ(_allObjects.size(), 1u);
	EXPECT_DOUBLE_EQ(_allObjects.front()->GetRect().x, shovedTo.x) << "the tank came up where the burst started";
}

// a wall cannot be shoved aside, so the burst only waits it out
TEST_F(TankSpawnerTest, ASpawnWaitsForItsSquareToBeFreed)
{
	_instantSpawnAnimationSubs.clear();

	std::vector<AnimationCreateTankSpawnEvent> bursts{};
	const EventSubscription burstSub{_events->AddListener(
			[&bursts](const AnimationCreateTankSpawnEvent& event) { bursts.push_back(event); })};

	const Uuid uuid{UuidUtils::GetRandomUuid()};
	_events->EmitEvent(RespawnTankEvent{.type = TankType::ENEMY1, .uuid = uuid});
	ASSERT_EQ(bursts.size(), 1u);

	const std::shared_ptr<BaseObj> squatter{
			SpawnObstacle(FPoint{.x = bursts.front().rect.x, .y = bursts.front().rect.y}, ObstacleType::Steel)};
	ASSERT_NE(squatter, nullptr);

	const auto isTank = [](const std::shared_ptr<BaseObj>& obj)
	{
		return std::dynamic_pointer_cast<Tank>(obj) != nullptr;
	};

	_events->EmitEvent(SpawnAnimationFinishedEvent{.uuid = uuid});

	EXPECT_EQ(std::ranges::count_if(_allObjects, isTank), 0) << "it landed on top of what stood in its square";
	EXPECT_EQ(bursts.size(), 2u) << "the burst ended while the tank waiting for it had nowhere to land";

	squatter->SetIsAlive(false);
	_events->EmitEvent(SpawnAnimationFinishedEvent{.uuid = uuid});

	EXPECT_EQ(std::ranges::count_if(_allObjects, isTank), 1) << "the square came free and nobody came up in it";
}

// a host runs its own clock, so its spawn burst is the counted-down one
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
	_events->EmitEvent(TankRespawnedEvent{.type = TankType::ENEMY1,
										  .model = TankModel::Basic,
										  .uuid = uuid,
										  .pos = {.x = 0.0, .y = 0.0}});

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
	_events->EmitEvent(TankRespawnedEvent{.type = TankType::ENEMY1,
										  .model = TankModel::Basic,
										  .uuid = uuid,
										  .pos = {.x = 0.0, .y = 0.0}});
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

	_events->EmitEvent(TankRespawnedEvent{.type = TankType::ENEMY1,
										  .model = TankModel::Basic,
										  .uuid = uuid,
										  .pos = cancelledPos});
	_events->EmitEvent(TankDiedEvent{.who = Author::Enemy1, .uuid = uuid, .author = Author::None});
	_events->EmitEvent(TankRespawnedEvent{.type = TankType::ENEMY1,
										  .model = TankModel::Basic,
										  .uuid = uuid,
										  .pos = currentPos});
	_events->EmitEvent(TankSpawnCompletedEvent{.uuid = uuid});

	ASSERT_EQ(_allObjects.size(), 1u);
	EXPECT_EQ(_allObjects.front()->GetRect().x, currentPos.x);
}

// the machine running the server has no seat of its own - its keyboard drives nothing
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

// a strip full of bush is passable ground, so it must not read as a sealed edge
TEST_F(TankSpawnerTest, AnEnemySpawnsOnBush)
{
	PlantTopRow(ObstacleType::Bush);

	const std::size_t before{CountTanksInTopRow()};
	_events->EmitEvent(RespawnTanksEvent{});

	EXPECT_GT(CountTanksInTopRow(), before) << "bush was read as a wall";
}

// ice is the same kind of ground, and the spawner treats it the same
TEST_F(TankSpawnerTest, AnEnemySpawnsOnIce)
{
	PlantTopRow(ObstacleType::Ice);

	const std::size_t before{CountTanksInTopRow()};
	_events->EmitEvent(RespawnTanksEvent{});

	EXPECT_GT(CountTanksInTopRow(), before) << "ice was read as a wall";
}

// water is not - a fresh tank has no ship bonus and could never drive off it
TEST_F(TankSpawnerTest, NoEnemySpawnsOnWater)
{
	PlantTopRow(ObstacleType::Water);

	const std::size_t before{CountTanksInTopRow()};
	_events->EmitEvent(RespawnTanksEvent{});

	EXPECT_EQ(CountTanksInTopRow(), before) << "a tank was put down on water";
}

// the one opening in the top row sits off both the tank and the half-tank stride, which a search by
// strides alone would miss
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

// no two seats can roll overlapping squares if each stays inside its quarter - so every roll is checked
TEST_F(TankSpawnerTest, EveryEnemyRollStaysInsideItsOwnQuarterOfTheFront)
{
	//NOTE: nothing lands, so the square stays the burst's - and the grenade is what frees it for the next roll
	_instantSpawnAnimationSubs.clear();

	ObjRectangle rolled{};
	const EventSubscription rolledSub{_events->AddListener(
			[&rolled](const AnimationCreateTankSpawnEvent& event) { rolled = event.rect; })};

	const double quarterWidth{static_cast<double>(_gameConfig.battlefieldSize.x) / 4.0};
	constexpr std::array seats{TankType::ENEMY1, TankType::ENEMY2, TankType::ENEMY3, TankType::ENEMY4};
	for (int round{}; round < 20; ++round)
	{
		for (const TankType type: seats)
		{
			_events->EmitEvent(RespawnTankEvent{.type = type, .uuid = UuidUtils::GetRandomUuid()});

			const auto quarter{static_cast<double>(type)};
			const std::string_view seat{ToString(SeatOf(type))};
			ASSERT_GE(rolled.x, quarter * quarterWidth) << seat << " rolled left of its quarter";
			ASSERT_LE(rolled.Right(), (quarter + 1.0) * quarterWidth) << seat << " rolled into the next quarter";

			_events->EmitEvent(Key(Faction::EnemyTeam), BonusGrenadePickupEvent{});
		}
	}
}

// and the whole front once its own quarter is walled off - out of place beats not coming up at all
TEST_F(TankSpawnerTest, AnEnemyLeavesItsQuarterOnlyWhenNothingFitsInIt)
{
	const double quarterWidth{static_cast<double>(_gameConfig.battlefieldSize.x) / 4.0};
	for (double x{0.0}; x < quarterWidth; x += _gameConfig.gridOffset)
	{
		SpawnObstacle(FPoint{.x = x, .y = 0.0}, ObstacleType::Steel);
	}

	_events->EmitEvent(RespawnTanksEvent{});

	EXPECT_EQ(CountTanksInTopRow(), 4u) << "the seat whose quarter was walled off never came up";
}

// 2P free-for-all keeps two bots on the field, not four
TEST_F(TankSpawnerTest, TwoPlayersFreeForAllStartsWithTwoPlayersAndTwoBots)
{
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::TwoPlayersFreeForAll, _respawnManager,
							 _tankSpawner);
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTanksEvent{});

	EXPECT_EQ(_allObjects.size(), 4u);
}

// a seat the match gave to a bot gets a coop tank, and the clients are told it is one
TEST_F(TankSpawnerTest, AHostSeatsABotWhereTheMatchSaysAndTellsTheClients)
{
	std::vector<TankType> told{};
	const EventSubscription toldSub{_events->AddListener([&told](const TankRespawnedEvent& event)
	{
		if (SlotOf(event.type))
		{
			told.push_back(event.type);
		}
	})};
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsHost, _respawnManager, _tankSpawner);
	_events->EmitEvent(SeatsFilledEvent{
			.holders = {SeatHolder::Player, SeatHolder::Bot, SeatHolder::Empty, SeatHolder::Empty}});
	_events->EmitEvent(GameResetEvent{});

	_events->EmitEvent(RespawnTanksEvent{});

	EXPECT_EQ(told, (std::vector{TankType::PLAYER1, TankType::COOP2}));
}

// the player who takes a bot's seat drives on the tank the bot left, where it stands
TEST_F(TankSpawnerTest, APlayerTakingOverABotDrivesItsTankWhereItStands)
{
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsHost, _respawnManager, _tankSpawner);
	_events->EmitEvent(SeatsFilledEvent{
			.holders = {SeatHolder::Player, SeatHolder::Bot, SeatHolder::Empty, SeatHolder::Empty}});
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTankEvent{.type = TankType::PLAYER2, .uuid = UuidUtils::GetRandomUuid()});
	ASSERT_EQ(_allObjects.size(), 1u);
	const std::shared_ptr<BaseObj> tank{_allObjects.front()};
	const Uuid uuid{tank->GetUuid()};

	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Bot});
	const FPoint standing{tank->GetPos()};
	_events->EmitEvent(TickUpdateEvent{.deltaTime = 1.0 / 60.0});
	ASSERT_EQ(standing, tank->GetPos()) << "the bot still drives the seat it gave up";

	_events->EmitEvent(Key(InputChannel::RemoteP2), MoveUpEvent{.isPressed = true});
	_events->EmitEvent(TickUpdateEvent{.deltaTime = 1.0 / 60.0});
	EXPECT_NE(standing, tank->GetPos()) << "the newcomer's keys do not reach the tank";
	EXPECT_EQ(uuid, tank->GetUuid());

	WorldSnapshot field{};
	_events->EmitEvent(WorldSnapshotRequestedEvent{.snapshot = field});
	ASSERT_EQ(field.tanks.size(), 1u);
	EXPECT_EQ(field.tanks.front().type, TankType::PLAYER2) << "the newcomer would be sent the seat as a bot's";
}

// the player who carried a tier left between the levels - whoever takes the seat afterwards starts afresh
TEST_F(TankSpawnerTest, ATierCarriedForASeatThatWentToABotIsNotHandedOn)
{
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsHost, _respawnManager, _tankSpawner);
	_events->EmitEvent(SeatsFilledEvent{
			.holders = {SeatHolder::Player, SeatHolder::Player, SeatHolder::Empty, SeatHolder::Empty}});
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTankEvent{.type = TankType::PLAYER2, .uuid = UuidUtils::GetRandomUuid()});
	_events->EmitEvent(Key(Author::Player2), BonusStarPickupEvent{});
	ASSERT_EQ(_allObjects.size(), 1u);
	ASSERT_EQ(std::dynamic_pointer_cast<Tank>(_allObjects.front())->GetTier(), 2u);
	_events->EmitEvent(NextLevelRequestedEvent{});

	_events->EmitEvent(SeatsFilledEvent{
			.holders = {SeatHolder::Player, SeatHolder::Bot, SeatHolder::Empty, SeatHolder::Empty}});
	_events->EmitEvent(GameResetEvent{.keepsPlayerProgress = true});
	_allObjects.clear();
	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Bot});
	_events->EmitEvent(RespawnTankEvent{.type = TankType::PLAYER2, .uuid = UuidUtils::GetRandomUuid()});

	ASSERT_EQ(_allObjects.size(), 1u);
	EXPECT_EQ(std::dynamic_pointer_cast<Tank>(_allObjects.front())->GetTier(), 1u)
			<< "the newcomer drove off on the tier of the player who left";
}

// a bot taking over from a player who left drives the tank left standing, and the seat's next tank is a bot's
TEST_F(TankSpawnerTest, ABotTakingOverFromAPlayerWhoLeftDrivesItsTank)
{
	std::vector<TankType> told{};
	const EventSubscription toldSub{_events->AddListener([&told](const TankRespawnedEvent& event)
	{
		told.push_back(event.type);
	})};
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsHost, _respawnManager, _tankSpawner);
	_events->EmitEvent(SeatsFilledEvent{
			.holders = {SeatHolder::Player, SeatHolder::Player, SeatHolder::Empty, SeatHolder::Empty}});
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTankEvent{.type = TankType::PLAYER2, .uuid = UuidUtils::GetRandomUuid()});
	ASSERT_EQ(_allObjects.size(), 1u);
	const auto tank{std::dynamic_pointer_cast<Tank>(_allObjects.front())};

	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Player,
											  .to = SeatHolder::Bot});

	WorldSnapshot field{};
	_events->EmitEvent(WorldSnapshotRequestedEvent{.snapshot = field});
	ASSERT_EQ(field.tanks.size(), 1u);
	EXPECT_EQ(field.tanks.front().uuid, tank->GetUuid());
	EXPECT_EQ(field.tanks.front().type, TankType::COOP2) << "the clients would be sent the seat as a player's";

	told.clear();
	_events->EmitEvent(RespawnTankEvent{.type = TankType::PLAYER2, .uuid = UuidUtils::GetRandomUuid()});
	EXPECT_EQ(told, std::vector{TankType::COOP2}) << "the seat's next tank waits for a player who is not there";
}

// a seat given up to nobody takes its tank off the field - no death, so nobody scores it and nothing bursts
TEST_F(TankSpawnerTest, ASeatGivenUpToNobodyTakesItsTankOffWithoutADeath)
{
	bool isDeathTold{};
	const EventSubscription deathSub{_events->AddListener([&isDeathTold](const TankDiedEvent&)
	{
		isDeathTold = true;
	})};
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsHost, _respawnManager, _tankSpawner);
	_events->EmitEvent(SeatsFilledEvent{
			.holders = {SeatHolder::Player, SeatHolder::Player, SeatHolder::Empty, SeatHolder::Empty}});
	_events->EmitEvent(GameResetEvent{});
	_events->EmitEvent(RespawnTankEvent{.type = TankType::PLAYER2, .uuid = UuidUtils::GetRandomUuid()});
	ASSERT_EQ(_allObjects.size(), 1u);
	const std::shared_ptr<BaseObj> tank{_allObjects.front()};

	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Player,
											  .to = SeatHolder::Empty});

	EXPECT_FALSE(tank->GetIsAlive()) << "the tank of the one who left still stands";
	EXPECT_FALSE(isDeathTold) << "taking the tank off was told as a death";
}

// nor does a tank of that seat still in its burst land afterwards
TEST_F(TankSpawnerTest, ASeatGivenUpToNobodyNeverLandsTheTankItWasSpawning)
{
	TestUtils::ApplyGameMode(_events, _allObjects, _gameConfig, GameMode::PlayAsHost, _respawnManager, _tankSpawner);
	_events->EmitEvent(SeatsFilledEvent{
			.holders = {SeatHolder::Player, SeatHolder::Player, SeatHolder::Empty, SeatHolder::Empty}});
	_events->EmitEvent(GameResetEvent{});
	_instantSpawnAnimationSubs.clear();
	const Uuid uuid{UuidUtils::GetRandomUuid()};
	_events->EmitEvent(RespawnTankEvent{.type = TankType::PLAYER2, .uuid = uuid});
	ASSERT_TRUE(_allObjects.empty()) << "the control failed - the tank landed with no burst to wait for";

	_events->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Player,
											  .to = SeatHolder::Empty});
	_events->EmitEvent(SpawnAnimationFinishedEvent{.uuid = uuid});

	EXPECT_TRUE(_allObjects.empty()) << "the seat given up landed a tank";
}
