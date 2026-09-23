#include "TestUtils.h"
#include "application/GameConfig.h"
#include "components/BonusSpawner.h"
#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "components/GameStatistics.h"
#include "components/ObstacleSpawner.h"
#include "components/StatisticsData.h"
#include "components/TankSpawner.h"
#include "components/WorldSnapshot.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/GameModeEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/SpawnEvents.h"
#include "components/events/StatisticsEvents.h"
#include "components/events/TimingEvents.h"
#include "components/managers/GameStateManager.h"
#include "components/managers/RespawnManager.h"
#include "components/managers/SpawnManager.h"
#include "components/managers/WorldScaleManager.h"
#include "entities/bonuses/Bonus.h"
#include "entities/obstacles/Obstacle.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/BulletResetProperty.h"
#include "entities/pawns/Tank.h"
#include "enums/Author.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "enums/GameState.h"
#include "enums/ObstacleType.h"
#include "enums/PlayerSlot.h"
#include "enums/RespawnGroup.h"
#include "enums/TankType.h"
#include "geometry/ObjRectangle.h"
#include "network/NetworkCommandQueue.h"
#include "network/ReplicationApplier.h"
#include "network/Serializer.h"
#include "network/commands/CommandBatch.h"
#include "utils/Uuid.h"
#include "gtest/gtest.h"
#include <algorithm>
#include <map>
#include <memory>
#include <variant>
#include <vector>

// The host's field crosses the wire as one command onto a client that has seen none of it
class WorldSnapshotTest : public testing::Test
{
protected:
	std::shared_ptr<EventSystem> _serverEvents{std::make_shared<EventSystem>()};
	GameConfig _hostConfig{};
	std::vector<std::shared_ptr<BaseObj>> _hostObjects;
	std::shared_ptr<RespawnManager> _hostRespawnManager{nullptr};
	std::shared_ptr<TankSpawner> _hostTankSpawner{nullptr};
	std::shared_ptr<BulletPool> _hostBulletPool{nullptr};
	std::unique_ptr<ObstacleSpawner> _hostObstacleSpawner{nullptr};
	std::unique_ptr<BonusSpawner> _hostBonusSpawner{nullptr};
	std::unique_ptr<GameStatistics> _hostStatistics{nullptr};
	std::unique_ptr<GameStateManager> _hostStateManager{nullptr};
	std::unique_ptr<WorldScaleManager> _hostWorldScale{nullptr};

	std::shared_ptr<EventSystem> _clientEvents{std::make_shared<EventSystem>()};
	GameConfig _clientConfig{};
	std::unique_ptr<SpawnManager> _clientSpawnManager{nullptr};
	std::unique_ptr<GameStatistics> _clientStatistics{nullptr};
	std::unique_ptr<GameStateManager> _clientStateManager{nullptr};
	std::unique_ptr<WorldScaleManager> _clientWorldScale{nullptr};
	//NOTE: weak - the client's SpawnManager owns its field, and what it lets go of has to be free to die
	std::vector<std::weak_ptr<BaseObj>> _clientSpawned{};

	std::map<TankType, Uuid> _hostTankSpawns{};
	std::vector<Uuid> _hostBonusSpawns{};
	std::map<RespawnGroup, unsigned short> _hostLives{};
	std::map<RespawnGroup, unsigned short> _clientLives{};
	std::map<Author, bool> _clientHelmets{};
	std::vector<EventSubscription> _subs{};

	void SetUp() override
	{
		_subs.push_back(TestUtils::WireSpawnQueue(_serverEvents, _hostObjects));
		_subs.push_back(TestUtils::WireWorldDisposal(_serverEvents, _hostObjects));
		TestUtils::ApplyGameMode(_serverEvents, _hostObjects, _hostConfig, GameMode::PlayAsHost, _hostRespawnManager,
								 _hostTankSpawner);
		_hostBulletPool = std::make_shared<BulletPool>(_serverEvents, _hostObjects, _hostConfig);
		_hostObstacleSpawner = std::make_unique<ObstacleSpawner>(_serverEvents, _hostConfig);
		_hostBonusSpawner = std::make_unique<BonusSpawner>(_serverEvents, _hostObjects, _hostConfig);
		_hostStatistics = std::make_unique<GameStatistics>(_serverEvents);
		_hostStateManager = std::make_unique<GameStateManager>(_serverEvents);
		_hostWorldScale = std::make_unique<WorldScaleManager>(_serverEvents, _hostConfig);
		_subs.push_back(_serverEvents->AddListener([this](const TankRespawnedEvent& event)
		{
			_hostTankSpawns.insert_or_assign(event.type, event.uuid);
		}));
		_subs.push_back(_serverEvents->AddListener([this](const BonusSpawnedEvent& event)
		{
			_hostBonusSpawns.push_back(event.uuid);
		}));
		_subs.push_back(_serverEvents->AddListener([this](const RespawnCountChangedToEvent& event)
		{
			_hostLives.insert_or_assign(event.group, event.respawnCount);
		}));

		_clientConfig.gameMode = GameMode::PlayAsClient;
		_clientSpawnManager = std::make_unique<SpawnManager>(_clientEvents, _clientConfig);
		_clientStatistics = std::make_unique<GameStatistics>(_clientEvents);
		_clientStateManager = std::make_unique<GameStateManager>(_clientEvents);
		_clientWorldScale = std::make_unique<WorldScaleManager>(_clientEvents, _clientConfig);
		_subs.push_back(_clientEvents->AddListener([this](const AddToSpawnQueueEvent& event)
		{
			_clientSpawned.push_back(event.obj);
		}));
		_subs.push_back(_clientEvents->AddListener([this](const RespawnCountChangedToEvent& event)
		{
			_clientLives.insert_or_assign(event.group, event.respawnCount);
		}));
		_subs.push_back(_clientEvents->AddListener([this](const AnimationBonusHelmetChangeEvent& event)
		{
			_clientHelmets.insert_or_assign(event.author, event.isEnable);
		}));

		_serverEvents->EmitEvent(GameModeAppliedEvent{.mode = GameMode::PlayAsHost});
		_serverEvents->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P1});
		_serverEvents->EmitEvent(ServerInClientReadyToStartGameEvent{.slot = PlayerSlot::P2});
		_serverEvents->EmitEvent(RespawnTanksEvent{});
	}

	//NOTE: the whole trip a late joiner's snapshot makes - taken, archived, read back and applied
	void Replicate() const
	{
		network::commands::CommandBatch batch;
		auto& snapshot{std::get<WorldSnapshot>(batch.commands.emplace_back(WorldSnapshot{}))};
		_serverEvents->EmitEvent(WorldSnapshotRequestedEvent{.snapshot = snapshot});

		const auto received{network::Deserialize(network::Serialize(batch))};
		ASSERT_TRUE(received.has_value());

		network::NetworkCommandQueue queue;
		network::commands::ReplicationApplier applier{_clientEvents, queue};
		std::ranges::for_each(received->commands, [&applier](const auto& command) { applier.Apply(command); });
		queue.ProcessAll();
	}

	void Land(const TankType type) const
	{
		_serverEvents->EmitEvent(SpawnAnimationFinishedEvent{.uuid = _hostTankSpawns.at(type)});
	}

	[[nodiscard]] std::shared_ptr<Tank> HostTank(const TankType type) const
	{
		const Uuid uuid{_hostTankSpawns.at(type)};
		const auto it{std::ranges::find(_hostObjects, uuid, &BaseObj::GetUuid)};

		return it == _hostObjects.end() ? nullptr : std::dynamic_pointer_cast<Tank>(*it);
	}

	[[nodiscard]] std::shared_ptr<BaseObj> ClientObject(const Uuid uuid) const
	{
		const auto it{std::ranges::find_if(_clientSpawned, [uuid](const std::weak_ptr<BaseObj>& spawned)
		{
			const auto object{spawned.lock()};
			return object && object->GetUuid() == uuid;
		})};

		return it == _clientSpawned.end() ? nullptr : it->lock();
	}
};

// the snapshot carries the phase, so a client joining mid-match lands in the one being played
TEST_F(WorldSnapshotTest, ThePhaseComesFromTheHost)
{
	Replicate();

	EXPECT_EQ(_clientStateManager->GetState(), GameState::Playing);
}

// A client never opens the map file, so the snapshot is the only thing that can tell it how wide the
// field is - and a client that guesses draws the map over its own side bar
TEST_F(WorldSnapshotTest, TheFieldsSizeComesFromTheHost)
{
	_serverEvents->EmitEvent(MapLoadedEvent{.cols = 61u, .rows = 50u});
	ASSERT_NE(_clientConfig.battlefieldSize, _hostConfig.battlefieldSize)
			<< "the control failed - the client already had the host's field size";

	Replicate();

	EXPECT_EQ(_clientConfig.battlefieldSize, _hostConfig.battlefieldSize);
}

// what the host has already lost is not in the snapshot, so the client does not build it
TEST_F(WorldSnapshotTest, AWallDestroyedOnTheHostIsNotRebuiltOnTheClient)
{
	const double cell{_hostConfig.gridOffset};
	_serverEvents->EmitEvent(SpawnObstacleEvent{.rect = {.x = 0.0, .y = 300.0, .w = cell, .h = cell},
											  .type = ObstacleType::Brick});
	_serverEvents->EmitEvent(SpawnObstacleEvent{.rect = {.x = cell, .y = 300.0, .w = cell, .h = cell},
											  .type = ObstacleType::Steel});
	const std::shared_ptr<BaseObj> shot{_hostObjects.at(_hostObjects.size() - 2u)};
	const std::shared_ptr<BaseObj> standing{_hostObjects.back()};
	shot->TakeDamage(1u, Author::Player1);
	_serverEvents->EmitEvent(PostTickUpdateEvent{});

	Replicate();

	EXPECT_EQ(ClientObject(shot->GetUuid()), nullptr);
	const auto steel{ClientObject(standing->GetUuid())};
	ASSERT_NE(steel, nullptr);
	EXPECT_EQ(steel->GetPos(), standing->GetPos());
	EXPECT_FALSE(steel->GetIsDestructible());
}

// a tank arrives whole: where it stands, where it faces, its health, its tier and its shield
TEST_F(WorldSnapshotTest, ALandedTankKeepsItsPositionDirectionHealthTierAndHelmet)
{
	Land(TankType::PLAYER1);
	const std::shared_ptr<Tank> host{HostTank(TankType::PLAYER1)};
	ASSERT_NE(host, nullptr);
	host->SetDirection(Direction::LEFT);
	host->TakeDamage(15u, Author::Enemy1);
	_serverEvents->EmitEvent(Key(Author::Player1), BonusStarPickupEvent{});
	_serverEvents->EmitEvent(Key(Author::Player1), BonusHelmetStatusChangeEvent{.isActive = true});

	Replicate();

	const auto client{std::dynamic_pointer_cast<Tank>(ClientObject(host->GetUuid()))};
	ASSERT_NE(client, nullptr);
	EXPECT_EQ(client->GetPos(), host->GetPos());
	EXPECT_EQ(client->GetDirection(), Direction::LEFT);
	EXPECT_EQ(client->GetHealth(), host->GetHealth());
	EXPECT_EQ(client->GetTier(), 2u);
	EXPECT_EQ(client->GetAuthor(), Author::Player1);
	EXPECT_TRUE(_clientHelmets.at(Author::Player1));
}

//NOTE: a landing tank puts its helmet on, so a helmet already gone on the host has to be taken off again
TEST_F(WorldSnapshotTest, AHelmetThatRanOutOnTheHostIsOffOnTheClient)
{
	Land(TankType::ENEMY2);

	Replicate();

	ASSERT_TRUE(_clientHelmets.contains(Author::Enemy2));
	EXPECT_FALSE(_clientHelmets.at(Author::Enemy2));
}

// one still in its spawn flash on the host keeps flashing until the host reports it landed
TEST_F(WorldSnapshotTest, ATankStillBurstingOnTheHostLandsOnlyWhenTheHostSaysSo)
{
	const Uuid bursting{_hostTankSpawns.at(TankType::PLAYER2)};

	Replicate();
	ASSERT_EQ(ClientObject(bursting), nullptr);

	_clientEvents->EmitEvent(TankSpawnCompletedEvent{.uuid = bursting});

	EXPECT_NE(ClientObject(bursting), nullptr);
}

//NOTE: the shooter is no use here - it may be dead already, or turned away from its own bullet
TEST_F(WorldSnapshotTest, ABulletInFlightIsRebuiltWhereItIsNowhereNearItsShooter)
{
	const BulletResetProperty property{.rect = {.x = 200.0, .y = 250.0, .w = 9.0, .h = 9.0},
									   .dir = Direction::RIGHT,
									   .health = 1,
									   .author = Author::Enemy3};
	const std::shared_ptr<Bullet> host{_hostBulletPool->SpawnBullet(property)};
	_serverEvents->EmitEvent(AddToSpawnQueueEvent{.obj = host});

	Replicate();

	const auto client{std::dynamic_pointer_cast<Bullet>(ClientObject(host->GetUuid()))};
	ASSERT_NE(client, nullptr);
	EXPECT_EQ(client->GetPos(), host->GetPos());
	EXPECT_EQ(client->GetDirection(), Direction::RIGHT);
	EXPECT_EQ(client->GetAuthor(), Author::Enemy3);
}

// the same for a bonus: the settled one is placed, the pending one waits
TEST_F(WorldSnapshotTest, ASettledBonusLandsAndAPendingOneWaitsForTheHost)
{
	const double size{static_cast<double>(_hostConfig.bonusSize)};
	_hostBonusSpawner->SpawnBonus({.x = 100.0, .y = 400.0, .w = size, .h = size}, BonusType::Star, {}, true);
	_hostBonusSpawner->SpawnBonus({.x = 300.0, .y = 400.0, .w = size, .h = size}, BonusType::Ship);
	const Uuid settled{_hostBonusSpawns.at(0u)};
	const Uuid pending{_hostBonusSpawns.at(1u)};
	_serverEvents->EmitEvent(SpawnAnimationFinishedEvent{.uuid = settled});

	Replicate();

	const auto bonus{std::dynamic_pointer_cast<Bonus>(ClientObject(settled))};
	ASSERT_NE(bonus, nullptr);
	EXPECT_TRUE(bonus->GetIsSuper());
	ASSERT_EQ(ClientObject(pending), nullptr);

	_clientEvents->EmitEvent(BonusSpawnCompletedEvent{.uuid = pending});
	EXPECT_NE(ClientObject(pending), nullptr);
}

//NOTE: taken over, not recounted - the client never saw the spawns and hits that made these numbers
TEST_F(WorldSnapshotTest, TheScoreAndTheLivesAreTakenOverAsTheyStand)
{
	_serverEvents->EmitEvent(BrickWallDiedEvent{.author = Author::Player2});
	_serverEvents->EmitEvent(StatisticsTankHitEvent{.who = Author::Enemy1, .author = Author::Player1});
	ASSERT_EQ(_hostLives.at(RespawnGroup::ENEMY_ALL), 16u);

	Replicate();

	const StatisticsData& client{_clientStatistics->GetData()};
	EXPECT_EQ(client.brickWallDiedByPlayerTwo, 1u);
	EXPECT_EQ(client.enemyHitByPlayerOne, 1u);
	EXPECT_EQ(_clientLives, _hostLives);
}

// a second snapshot replaces the field rather than laying another one over it
TEST_F(WorldSnapshotTest, ASnapshotReplacesTheFieldTheClientHadInsteadOfAddingToIt)
{
	_clientEvents->EmitEvent(
			ObstacleSpawnedEvent{.pos = {.x = 48.0, .y = 48.0}, .type = ObstacleType::Brick, .uuid = {}});
	ASSERT_EQ(_clientSpawned.size(), 1u);
	const std::weak_ptr<BaseObj> stale{_clientSpawned.front()};

	Replicate();

	EXPECT_TRUE(stale.expired()) << "the client kept a wall the host never had";
}
