#include "geometry/Point.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/StatisticsEvents.h"
#include "components/events/TimingEvents.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/DespawnReason.h"
#include "enums/DisconnectReason.h"
#include "enums/ObstacleType.h"
#include "enums/TankType.h"
#include "network/ClientHandler.h"
#include "network/CommandDispatcher.h"
#include "network/Serializer.h"
#include "network/commands/CommandBatch.h"
#include "network/commands/Disconnect.h"
#include "network/ServerHandler.h"
#include "gtest/gtest.h"
#include "utils/Uuid.h"
#include <array>
#include <chrono>
#include <memory>
#include <optional>
#include <thread>
#include <vector>
#include "utils/UuidUtils.h"
#include "TestUtils.h"//NOTE: PrintTo for the Point types

class NetworkTest : public testing::Test
{
protected:
	Uuid _uuid{UuidUtils::GetUuidFromString("01234567-89ab-cdef-0123-456789abcdef")};

	void SetUp() override {}

	void TearDown() override {}

	//NOTE: one bus per node - on a shared bus a listener fires off the local emit before anything
	//crosses the wire, and the test passes with no networking at all
	std::shared_ptr<EventSystem> _hostEvents{std::make_shared<EventSystem>()};
	std::shared_ptr<EventSystem> _clientEvents{std::make_shared<EventSystem>()};
	double _deltaTimeOneFrame{1.0 / 60.0};

	//NOTE: stands in for MainLoop, in its order - nothing is received or sent without it
	void Pump() const
	{
		_hostEvents->EmitEvent(NetCommandUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		_clientEvents->EmitEvent(NetCommandUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		_hostEvents->EmitEvent(PreTickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		_clientEvents->EmitEvent(PreTickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		_hostEvents->EmitEvent(NetworkEndFrameEvent{});
		_clientEvents->EmitEvent(NetworkEndFrameEvent{});
	}

	//NOTE: listeners fire from the drain Pump does, so on this thread - a captured variable is enough
	template<typename Predicate>
	[[nodiscard]] bool PumpUntil(Predicate predicate, const std::chrono::milliseconds timeout = kWaitTimeout) const
	{
		const auto deadline = std::chrono::steady_clock::now() + timeout;
		while (!predicate() && std::chrono::steady_clock::now() < deadline)
		{
			Pump();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}

		return predicate();
	}

	//NOTE: port 0 - the OS picks a free one, so a leftover socket cannot collide
	[[nodiscard]] std::unique_ptr<network::commands::ServerHandler> MakeHost() const
	{
		return std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, _hostEvents);
	}

	[[nodiscard]] std::unique_ptr<network::commands::ClientHandler> MakeClient(const uint16_t port) const
	{
		return std::make_unique<network::commands::ClientHandler>("127.0.0.1", port, _clientEvents);
	}

	//NOTE: keeps pumping while it waits - the client's own retries need it
	[[nodiscard]] bool ReboundHost(std::unique_ptr<network::commands::ServerHandler>& server,
								   const uint16_t port) const
	{
		return PumpUntil([this, &server, port]
		{
			if (server)
			{
				return true;
			}

			try
			{
				server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", port, _hostEvents);
			}
			catch (const std::exception&)
			{
				return false;
			}

			return true;
		}, kRebindTimeout);
	}

	static constexpr std::chrono::milliseconds kWaitTimeout{5000};
	//NOTE: short on purpose - the client gives up after ~5s, and a slow re-bind eats that window
	static constexpr std::chrono::milliseconds kRebindTimeout{1500};
};


TEST_F(NetworkTest, PosEventReplication)
{
	const auto server = MakeHost();
	const auto client = MakeClient(server->GetBoundPort());
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	constexpr FPoint posOrigin{.x = 42.0, .y = 42.0};
	constexpr auto directionOrigin{Direction::UP};

	std::optional<PosChangedEvent> received{};
	auto posSub = _clientEvents->AddListener(Key(_uuid),
			[&received](const PosChangedEvent& event) { received = event; });

	_hostEvents->EmitEvent(
			PosChangedEvent{.who = "TestTank", .pos = posOrigin, .dir = directionOrigin, .uuid = _uuid});

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); }));
	EXPECT_EQ(posOrigin, received->pos);
	EXPECT_EQ(directionOrigin, received->dir);
}

TEST_F(NetworkTest, ShotEventReplication)
{
	const auto server = MakeHost();
	const auto client = MakeClient(server->GetBoundPort());
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	constexpr Direction direction{Direction::UP};
	const auto name{std::string("TestTank")};

	std::optional<TankShotEvent> received{};
	auto shotSub = _clientEvents->AddListener(Key(name),
			[&received](const TankShotEvent& event) { received = event; });

	_hostEvents->EmitEvent(TankShotEvent{.who = name, .dir = direction, .bulletUuid = _uuid});

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); }));
	EXPECT_EQ(direction, received->dir);
	EXPECT_EQ(_uuid, received->bulletUuid);
}

TEST_F(NetworkTest, HealthEventReplication)
{
	const auto server = MakeHost();
	const auto client = MakeClient(server->GetBoundPort());
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	constexpr int healthOrigin{42};

	std::optional<int> received{};
	auto healthSub = _clientEvents->AddListener(Key(_uuid),
			[&received](const HealthChangedEvent& event) { received = event.health; });

	_hostEvents->EmitEvent(HealthChangedEvent{.who = "TestTank", .health = healthOrigin, .uuid = _uuid});

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); }));
	EXPECT_EQ(healthOrigin, *received);
}

//NOTE: two despawns - a destroyed bullet and a picked-up bonus travel the same command
TEST_F(NetworkTest, DespawnEventReplication)
{
	const auto server = MakeHost();
	const auto client = MakeClient(server->GetBoundPort());
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	std::vector<DespawnedEvent> received{};
	auto despawnSub = _clientEvents->AddListener(Key(_uuid),
			[&received](const DespawnedEvent& event) { received.push_back(event); });

	_hostEvents->EmitEvent(DespawnedEvent{.who = "Bullet", .uuid = _uuid, .reason = DespawnReason::Destroyed});
	_hostEvents->EmitEvent(
			DespawnedEvent{.who = "BonusHelmet", .uuid = _uuid, .reason = DespawnReason::PickedUp});

	ASSERT_TRUE(PumpUntil([&received] { return received.size() == 2u; }));
	EXPECT_EQ(_uuid, received[0].uuid);
	EXPECT_EQ(DespawnReason::Destroyed, received[0].reason);
	EXPECT_EQ(DespawnReason::PickedUp, received[1].reason);
}

//TODO: cover all statistics items like this
TEST_F(NetworkTest, StatisticsEventReplication)
{
	const auto server = MakeHost();
	const auto client = MakeClient(server->GetBoundPort());
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	std::optional<StatisticsBulletHitEvent> received{};
	auto statsSub = _clientEvents->AddListener(
			[&received](const StatisticsBulletHitEvent& event) { received = event; });

	_hostEvents->EmitEvent(StatisticsBulletHitEvent{.author = "author", .faction = Faction::EnemyTeam});

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); }));
	EXPECT_EQ("author", received->author);
	EXPECT_EQ(Faction::EnemyTeam, received->faction);
}

TEST_F(NetworkTest, PauseRequestFromClientPausesHost)
{
	const auto server = MakeHost();
	const auto client = MakeClient(server->GetBoundPort());
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	bool hostPauseToggled{false};
	auto pauseSub = _hostEvents->AddListener(
			[&hostPauseToggled](const PauseReleasedEvent&) { hostPauseToggled = true; });

	_clientEvents->EmitEvent(PauseRequestedEvent{.isPaused = true});

	EXPECT_TRUE(PumpUntil([&hostPauseToggled] { return hostPauseToggled; }));
}

TEST_F(NetworkTest, BonusSpawnEventReplication)
{
	const auto server = MakeHost();
	const auto client = MakeClient(server->GetBoundPort());
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	constexpr FPoint pos{.x = 42.0, .y = 42.0};
	constexpr auto type{BonusType::Timer};

	std::optional<BonusSpawnedEvent> received{};
	auto bonusSpawnSub = _clientEvents->AddListener(
			[&received](const BonusSpawnedEvent& event) { received = event; });

	_hostEvents->EmitEvent(BonusSpawnedEvent{.pos = pos, .type = type, .uuid = _uuid, .isSuper = true});

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); }));
	EXPECT_EQ(pos, received->pos);
	EXPECT_EQ(type, received->type);
	EXPECT_EQ(_uuid, received->uuid);
	EXPECT_TRUE(received->isSuper);
}

//NOTE: what makes the bonus real on the client - its own burst only draws
TEST_F(NetworkTest, BonusSpawnCompleteEventReplication)
{
	const auto server = MakeHost();
	const auto client = MakeClient(server->GetBoundPort());
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	std::optional<Uuid> received{};
	auto bonusSpawnCompleteSub = _clientEvents->AddListener(
			[&received](const BonusSpawnCompletedEvent& event) { received = event.uuid; });

	_hostEvents->EmitEvent(BonusSpawnCompletedEvent{.uuid = _uuid});

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); }));
	EXPECT_EQ(_uuid, *received);
}

TEST_F(NetworkTest, BonusStatusEventReplication)
{
	const auto server = MakeHost();
	const auto client = MakeClient(server->GetBoundPort());
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	const auto nameOrigin{std::string("Player1")};
	constexpr bool isActiveOrigin{true};

	std::optional<bool> received{};
	auto bonusStatusSub = _clientEvents->AddListener(Key(nameOrigin),
			[&received](const BonusHelmetAppliedEvent& event) { received = event.isActive; });

	_hostEvents->EmitEvent(BonusHelmetAppliedEvent{.name = nameOrigin, .isActive = isActiveOrigin});

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); }));
	EXPECT_EQ(isActiveOrigin, *received);
}

//NOTE: the tier travels as a result, on its own command - the client sets it rather than replaying
//the upgrade formula, exactly as it does with health
TEST_F(NetworkTest, TierEventReplication)
{
	const auto server = MakeHost();
	const auto client = MakeClient(server->GetBoundPort());
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	constexpr unsigned short tierOrigin{4u};

	std::optional<unsigned short> received{};
	auto tierSub = _clientEvents->AddListener(Key(_uuid),
			[&received](const TierChangedEvent& event) { received = event.tier; });

	_hostEvents->EmitEvent(TierChangedEvent{.who = "Player1", .tier = tierOrigin, .uuid = _uuid});

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); }));
	EXPECT_EQ(tierOrigin, *received);
}

//NOTE: no payload of its own - under test is that its alternative reaches the right name
TEST_F(NetworkTest, BonusShipStatusEventReplication)
{
	const auto server = MakeHost();
	const auto client = MakeClient(server->GetBoundPort());
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	const auto nameOrigin{std::string("Player1")};

	bool received{false};
	auto bonusShipSub = _clientEvents->AddListener(Key(nameOrigin),
			[&received](const BonusShipAppliedEvent&) { received = true; });

	_hostEvents->EmitEvent(BonusShipAppliedEvent{.name = nameOrigin});

	EXPECT_TRUE(PumpUntil([&received] { return received; }));
}

TEST_F(NetworkTest, ObstacleSpawnEventReplication)
{
	const auto server = MakeHost();
	const auto client = MakeClient(server->GetBoundPort());
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	constexpr auto obstacleType = ObstacleType::Brick;
	constexpr FPoint posOrigin{.x = 42.0, .y = 43.0};

	std::optional<ObstacleSpawnedEvent> received{};
	auto obstacleSpawnSub = _clientEvents->AddListener(
			[&received](const ObstacleSpawnedEvent& event) { received = event; });

	_hostEvents->EmitEvent(ObstacleSpawnedEvent{.pos = posOrigin, .type = obstacleType, .uuid = _uuid});

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); }));
	EXPECT_EQ(posOrigin, received->pos);
	EXPECT_EQ(obstacleType, received->type);
	EXPECT_EQ(_uuid, received->uuid);
}

//NOTE: a whole map in one batch - one frame, and every command has to come out of it in order
TEST_F(NetworkTest, MassiveObstacleSpawnEventReplication)
{
	const auto server = MakeHost();
	const auto client = MakeClient(server->GetBoundPort());
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	constexpr auto obstacleType = ObstacleType::Brick;
	constexpr unsigned short itemsInMassiveTest = 10000u;

	std::vector<FPoint> sent{};
	sent.reserve(itemsInMassiveTest);
	for (size_t i = 0u; i < itemsInMassiveTest; ++i)
	{
		const auto value = static_cast<double>(i);
		sent.emplace_back(FPoint{.x = value, .y = value + 1.0});
	}

	std::vector<ObstacleSpawnedEvent> received{};
	received.reserve(itemsInMassiveTest);
	auto massiveObstacleSub = _clientEvents->AddListener(
			[&received](const ObstacleSpawnedEvent& event) { received.push_back(event); });

	for (const auto& pos: sent)
	{
		_hostEvents->EmitEvent(ObstacleSpawnedEvent{.pos = pos, .type = obstacleType, .uuid = _uuid});
	}

	ASSERT_TRUE(PumpUntil([&received] { return received.size() == itemsInMassiveTest; }))
			<< "only " << received.size() << " of " << itemsInMassiveTest << " obstacles arrived";

	for (size_t i = 0u; i < itemsInMassiveTest; ++i)
	{
		ASSERT_EQ(sent[i], received[i].pos) << "obstacle " << i << " arrived out of order or damaged";
		ASSERT_EQ(obstacleType, received[i].type);
		ASSERT_EQ(_uuid, received[i].uuid);
	}
}

TEST_F(NetworkTest, RespawnTankEventReplication)
{
	const auto server = MakeHost();
	const auto client = MakeClient(server->GetBoundPort());
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	constexpr std::array tankTypes{
			TankType::PLAYER1,
			TankType::PLAYER2,
			TankType::ENEMY1,
			TankType::ENEMY2,
			TankType::ENEMY3,
			TankType::ENEMY4
	};

	constexpr FPoint posOrigin{.x = 12.0, .y = 34.0};
	constexpr size_t expectedCount{tankTypes.size()};

	std::vector<TankRespawnedEvent> received{};
	auto respawnTankSub = _clientEvents->AddListener(
			[&received](const TankRespawnedEvent& event) { received.push_back(event); });

	for (const auto tankType: tankTypes)
	{
		_hostEvents->EmitEvent(TankRespawnedEvent{.type = tankType, .uuid = _uuid, .pos = posOrigin});
	}

	ASSERT_TRUE(PumpUntil([&received] { return received.size() == expectedCount; }));
	for (size_t i = 0u; i < tankTypes.size(); ++i)
	{
		EXPECT_EQ(tankTypes[i], received[i].type);
		EXPECT_EQ(_uuid, received[i].uuid);
		EXPECT_EQ(posOrigin, received[i].pos);
	}
}

//NOTE: the link and what rides it - a host still thinking the old client plays reconnects nothing
TEST_F(NetworkTest, ClientReconnectsAfterEstablishedLinkDrops)
{
	auto server = MakeHost();
	const uint16_t port = server->GetBoundPort();
	const auto client = MakeClient(port);

	int readySignals{0};
	int linksUp{0};
	std::vector<EventSubscription> subs{};
	subs.push_back(_hostEvents->AddListener(
			[&readySignals](const ServerInClientReadyToStartGameEvent&) { ++readySignals; }));
	subs.push_back(_clientEvents->AddListener([&linksUp](const ClientConnectedToHostEvent&) { ++linksUp; }));

	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));
	ASSERT_TRUE(PumpUntil([&readySignals] { return readySignals == 1; })) << "host never got the first ready";

	//NOTE: Abort, not just reset - an announced leave stops the reconnect, and this test wants one
	server->Abort();
	server.reset();

	ASSERT_TRUE(PumpUntil([&client] { return !client->IsConnected(); }))
			<< "client never noticed the link dropped";

	if (!ReboundHost(server, port))
	{
		GTEST_SKIP() << "port " << port << " still held by the OS - nothing to test against";
	}

	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }, std::chrono::milliseconds{10000}))
			<< "client did not reconnect after the host came back";

	//NOTE: pumped, not read straight off - IsConnected flips before the event behind it is drained
	EXPECT_TRUE(PumpUntil([&linksUp] { return linksUp == 2; }))
			<< "client did not tell its own side the link was back";

	EXPECT_TRUE(PumpUntil([&readySignals] { return readySignals == 2; }))
			<< "reconnected client never asked the host to start the match again";
}

//NOTE: the other half of ClientQuitTellsHostWhy, and the case the reconnect test cannot cover -
//here the host outlives the loss and has to take the next client on the same acceptor
TEST_F(NetworkTest, HostLearnsTheClientDroppedWithoutSayingGoodbye)
{
	const auto server = MakeHost();
	const uint16_t port = server->GetBoundPort();
	auto client = MakeClient(port);

	bool clientLost{false};
	int readySignals{0};
	std::optional<DisconnectReason> announced{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_hostEvents->AddListener([&clientLost](const ServerClientLostEvent&) { clientLost = true; }));
	subs.push_back(_hostEvents->AddListener(
			[&readySignals](const ServerInClientReadyToStartGameEvent&) { ++readySignals; }));
	subs.push_back(_hostEvents->AddListener(
			[&announced](const ServerInDisconnectEvent& event) { announced = event.reason; }));

	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));
	ASSERT_TRUE(PumpUntil([&readySignals] { return readySignals == 1; }));

	client->Abort();
	client.reset();

	ASSERT_TRUE(PumpUntil([&clientLost] { return clientLost; }))
			<< "host never noticed the client stopped answering";
	EXPECT_FALSE(announced.has_value()) << "a dropped link reported itself as an announced leave";

	const auto secondClient = MakeClient(port);
	ASSERT_TRUE(PumpUntil([&secondClient] { return secondClient->IsConnected(); }))
			<< "host stopped accepting after losing the first client";
	EXPECT_TRUE(PumpUntil([&readySignals] { return readySignals == 2; }))
			<< "host never got a ready from the client that replaced the lost one";
}

//NOTE: like the reconnection test above - about the link, not about a command riding it
TEST_F(NetworkTest, HostShutdownTellsClientWhyAndStopsTheReconnect)
{
	auto server = MakeHost();
	const uint16_t port = server->GetBoundPort();
	const auto client = MakeClient(port);

	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	std::optional<DisconnectReason> received{};
	bool gaveUp{false};
	std::vector<EventSubscription> subs{};
	subs.push_back(_clientEvents->AddListener(
			[&received](const ClientInDisconnectEvent& event) { received = event.reason; }));
	subs.push_back(_clientEvents->AddListener([&gaveUp](const ClientReconnectAbandonedEvent&) { gaveUp = true; }));

	server.reset();//NOTE: the goodbye goes out from inside the destructor, before the socket closes

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); })) << "client never got the host's goodbye";
	EXPECT_EQ(DisconnectReason::HostShutdown, *received);

	//NOTE: comes from the branch that decides not to reconnect - no need to outwait a retry period
	ASSERT_TRUE(PumpUntil([&gaveUp] { return gaveUp; }))
			<< "client never gave up on the host that said it was leaving on purpose";

	if (!ReboundHost(server, port))
	{
		GTEST_SKIP() << "port " << port << " still held by the OS - nothing to test against";
	}

	Pump();
	EXPECT_FALSE(client->IsConnected()) << "client reconnected after the host said it was leaving on purpose";
}

TEST_F(NetworkTest, ClientQuitTellsHostWhy)
{
	const auto server = MakeHost();
	auto client = MakeClient(server->GetBoundPort());

	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	std::optional<DisconnectReason> received{};
	bool clientLost{false};
	std::vector<EventSubscription> subs{};
	subs.push_back(_hostEvents->AddListener(
			[&received](const ServerInDisconnectEvent& event) { received = event.reason; }));
	subs.push_back(_hostEvents->AddListener([&clientLost](const ServerClientLostEvent&) { clientLost = true; }));

	client.reset();

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); })) << "host never got the client's goodbye";
	EXPECT_EQ(DisconnectReason::PlayerQuit, *received);
	EXPECT_FALSE(clientLost) << "the EOF behind the goodbye was reported as a second, silent drop";
}

//TODO: other bonus effect replication test after write this replication
// TEST_F(NetworkTest, bonusKind...EventReplication) {

TEST(CommandDispatcherTest, UnreadableFrameIsReportedNotSwallowed)
{
	network::CommandDispatcher dispatcher{"test"};

	const std::string frame{"not an archive at all"};
	const auto dispatched = dispatcher.Dispatch(frame);

	ASSERT_FALSE(dispatched.has_value());
	EXPECT_EQ(dispatched.error().frameSize, frame.size());
	EXPECT_FALSE(dispatched.error().reason.empty());
}

TEST(CommandDispatcherTest, RegisteredHandlerRunsOnAGoodFrame)
{
	network::CommandDispatcher dispatcher{"test"};

	std::optional<DisconnectReason> seen{};
	dispatcher.RegisterAll({{CommandType::DISCONNECT,
							 [&seen](const network::commands::AnyCommand& command)
							 { seen = std::get<network::commands::Disconnect>(command).reason; }}});

	network::commands::CommandBatch batch;
	batch.commands.emplace_back(network::commands::Disconnect{.reason = DisconnectReason::GameOver});

	EXPECT_TRUE(dispatcher.Dispatch(network::Serialize(batch)).has_value());
	ASSERT_TRUE(seen.has_value());
	EXPECT_EQ(*seen, DisconnectReason::GameOver);
}

TEST(CommandDispatcherTest, CommandWithNoHandlerIsNotAFailure)
{
	network::CommandDispatcher dispatcher{"test"};

	network::commands::CommandBatch batch;
	batch.commands.emplace_back(network::commands::Disconnect{.reason = DisconnectReason::GameOver});

	EXPECT_TRUE(dispatcher.Dispatch(network::Serialize(batch)).has_value());
}
