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
#include "enums/InputChannel.h"
#include "enums/ObstacleType.h"
#include "enums/PlayerSlot.h"
#include "enums/TankType.h"
#include "network/ClientNode.h"
#include "network/MessageFraming.h"
#include "network/Serializer.h"
#include "network/commands/CommandBatch.h"
#include "network/commands/Disconnect.h"
#include "network/ServerNode.h"
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
	//NOTE: a seat is handed out per connection, so telling the two apart takes a second game process
	std::shared_ptr<EventSystem> _secondClientEvents{std::make_shared<EventSystem>()};
	double _deltaTimeOneFrame{1.0 / 60.0};

	//NOTE: stands in for MainLoop, in its order - nothing is received or sent without it
	void Pump() const
	{
		_hostEvents->EmitEvent(NetCommandUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		_clientEvents->EmitEvent(NetCommandUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		_secondClientEvents->EmitEvent(NetCommandUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		_hostEvents->EmitEvent(PreTickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		_clientEvents->EmitEvent(PreTickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		_secondClientEvents->EmitEvent(PreTickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		_hostEvents->EmitEvent(NetworkEndFrameEvent{});
		_clientEvents->EmitEvent(NetworkEndFrameEvent{});
		_secondClientEvents->EmitEvent(NetworkEndFrameEvent{});
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
	[[nodiscard]] std::unique_ptr<network::commands::ServerNode> MakeHost() const
	{
		return std::make_unique<network::commands::ServerNode>("127.0.0.1", 0, _hostEvents);
	}

	[[nodiscard]] std::unique_ptr<network::commands::ClientNode> MakeClient(const uint16_t port) const
	{
		return std::make_unique<network::commands::ClientNode>("127.0.0.1", port, _clientEvents);
	}

	//NOTE: keeps pumping while it waits - the client's own retries need it
	[[nodiscard]] bool ReboundHost(std::unique_ptr<network::commands::ServerNode>& server,
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
				server = std::make_unique<network::commands::ServerNode>("127.0.0.1", port, _hostEvents);
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
			PosChangedEvent{.pos = posOrigin, .dir = directionOrigin, .uuid = _uuid});

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); }));
	const auto& event = received.value();
	EXPECT_EQ(posOrigin, event.pos);
	EXPECT_EQ(directionOrigin, event.dir);
}

TEST_F(NetworkTest, ShotEventReplication)
{
	const auto server = MakeHost();
	const auto client = MakeClient(server->GetBoundPort());
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	constexpr Direction direction{Direction::UP};
	constexpr auto who{Author::Player1};

	std::optional<TankShotEvent> received{};
	auto shotSub = _clientEvents->AddListener(Key(who),
											  [&received](const TankShotEvent& event) { received = event; });

	_hostEvents->EmitEvent(TankShotEvent{.who = who, .dir = direction, .bulletUuid = _uuid});

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); }));
	const auto& event = received.value();
	EXPECT_EQ(direction, event.dir);
	EXPECT_EQ(_uuid, event.bulletUuid);
}

TEST_F(NetworkTest, HealthEventReplication)
{
	const auto server = MakeHost();
	const auto client = MakeClient(server->GetBoundPort());
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	constexpr int healthOrigin{42};

	std::optional<int> received{};
	auto healthSub = _clientEvents->AddListener(Key(_uuid),
												[&received](const HealthChangedEvent& event)
												{
													received = event.health;
												});

	_hostEvents->EmitEvent(HealthChangedEvent{.health = healthOrigin, .uuid = _uuid});

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
												 [&received](const DespawnedEvent& event)
												 {
													 received.push_back(event);
												 });

	_hostEvents->EmitEvent(DespawnedEvent{.uuid = _uuid, .reason = DespawnReason::Destroyed});
	_hostEvents->EmitEvent(DespawnedEvent{.uuid = _uuid, .reason = DespawnReason::PickedUp});

	ASSERT_TRUE(PumpUntil([&received] { return received.size() == 2u; }));
	EXPECT_EQ(_uuid, received[0].uuid);
	EXPECT_EQ(DespawnReason::Destroyed, received[0].reason);
	EXPECT_EQ(DespawnReason::PickedUp, received[1].reason);
}

TEST_F(NetworkTest, StatisticsEventReplication)
{
	const auto server = MakeHost();
	const auto client = MakeClient(server->GetBoundPort());
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	std::optional<StatisticsBulletHitEvent> received{};
	auto statsSub = _clientEvents->AddListener(
			[&received](const StatisticsBulletHitEvent& event) { received = event; });

	_hostEvents->EmitEvent(StatisticsBulletHitEvent{.author = Author::Enemy1});

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); }));
	EXPECT_EQ(Author::Enemy1, received.value().author);
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
	const auto& event = received.value();
	EXPECT_EQ(pos, event.pos);
	EXPECT_EQ(type, event.type);
	EXPECT_EQ(_uuid, event.uuid);
	EXPECT_TRUE(event.isSuper);
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

	constexpr auto authorOrigin{Author::Player1};
	constexpr bool isActiveOrigin{true};

	std::optional<bool> received{};
	auto bonusStatusSub = _clientEvents->AddListener(Key(authorOrigin),
													 [&received](const BonusHelmetAppliedEvent& event)
													 {
														 received = event.isActive;
													 });

	_hostEvents->EmitEvent(BonusHelmetAppliedEvent{.author = authorOrigin, .isActive = isActiveOrigin});

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

	_hostEvents->EmitEvent(TierChangedEvent{.tier = tierOrigin, .uuid = _uuid});

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); }));
	EXPECT_EQ(tierOrigin, *received);
}

//NOTE: no payload of its own - under test is that its alternative reaches the right seat
TEST_F(NetworkTest, BonusShipStatusEventReplication)
{
	const auto server = MakeHost();
	const auto client = MakeClient(server->GetBoundPort());
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	constexpr auto authorOrigin{Author::Player1};

	bool received{false};
	auto bonusShipSub = _clientEvents->AddListener(Key(authorOrigin),
												   [&received](const BonusShipAppliedEvent&) { received = true; });

	_hostEvents->EmitEvent(BonusShipAppliedEvent{.author = authorOrigin});

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
	const auto& event = received.value();
	EXPECT_EQ(posOrigin, event.pos);
	EXPECT_EQ(obstacleType, event.type);
	EXPECT_EQ(_uuid, event.uuid);
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

	//NOTE: Abort, not just reset - this test is about a link that dies without a goodbye
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
TEST_F(NetworkTest, HostShutdownTellsClientWhyAndKeepsTheReconnect)
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

	server.reset();//NOTE: the goodbye goes out from inside the destructor before the socket closes

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); })) << "client never got the host's goodbye";
	EXPECT_EQ(DisconnectReason::HostShutdown, *received);

	if (!ReboundHost(server, port))
	{
		GTEST_SKIP() << "port " << port << " still held by the OS - nothing to test against";
	}

	EXPECT_TRUE(PumpUntil([&client] { return client->IsConnected(); }, std::chrono::milliseconds{10000}))
			<< "client did not dial back the host that only announced a restart";
	EXPECT_FALSE(gaveUp) << "client gave up on a host that was restarting the same mode";
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

// Three effects ride BonusStatus and bonusType is all that tells them apart, so a swapped case arrives
// as the wrong effect on the wrong seat in silence. The ship lands keyed on its seat, the tank broadcasts
TEST_F(NetworkTest, ShipAndTankEffectsKeepTheirOwnEventAcrossTheWire)
{
	const auto server = MakeHost();
	const auto client = MakeClient(server->GetBoundPort());
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	constexpr auto shipSeat{Author::Player1};
	constexpr auto tankSeat{Author::Player2};

	std::optional<Author> ship{};
	std::optional<Author> tank{};

	std::vector<EventSubscription> subs{};
	subs.push_back(_clientEvents->AddListener(Key(shipSeat),
											  [&ship](const BonusShipAppliedEvent& event) { ship = event.author; }));
	subs.push_back(_clientEvents->AddListener(
			[&tank](const BonusTankAppliedEvent& event) { tank = event.author; }));

	_hostEvents->EmitEvent(BonusShipAppliedEvent{.author = shipSeat});
	_hostEvents->EmitEvent(BonusTankAppliedEvent{.author = tankSeat});

	ASSERT_TRUE(PumpUntil([&ship, &tank] { return ship.has_value() && tank.has_value(); }));
	EXPECT_EQ(shipSeat, *ship);
	EXPECT_EQ(tankSeat, *tank);
}

// The seat handed out on connect is the only thing tying a key to a tank: the server reads whose
// press it is off the session it arrived on.
TEST_F(NetworkTest, TheServerHandsOutTheSeatsInOrder)
{
	std::optional<PlayerSlot> first{};
	std::optional<PlayerSlot> second{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_clientEvents->AddListener([&first](const PlayerSlotAssignedEvent& e) { first = e.slot; }));
	subs.push_back(_secondClientEvents->AddListener([&second](const PlayerSlotAssignedEvent& e) { second = e.slot; }));

	const auto server = MakeHost();
	const auto client = MakeClient(server->GetBoundPort());
	ASSERT_TRUE(PumpUntil([&first] { return first.has_value(); })) << "the first client was told no seat";
	//NOTE: dialled only once the first seat is out, so the accept order is the test's, not the OS's
	const auto secondClient = std::make_unique<network::commands::ClientNode>("127.0.0.1",
																				 server->GetBoundPort(),
																				 _secondClientEvents);
	ASSERT_TRUE(PumpUntil([&second] { return second.has_value(); })) << "the second client was told no seat";

	EXPECT_EQ(PlayerSlot::P1, *first);
	EXPECT_EQ(PlayerSlot::P2, *second);
}

// A seat is held by its session and comes back only once that session is swept - and a session
// that said goodbye is swept on its own IsFinished verdict.
TEST_F(NetworkTest, ASeatComesBackWhenItsClientSaysGoodbye)
{
	std::optional<PlayerSlot> first{};
	std::optional<PlayerSlot> second{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_clientEvents->AddListener([&first](const PlayerSlotAssignedEvent& e) { first = e.slot; }));
	subs.push_back(_secondClientEvents->AddListener([&second](const PlayerSlotAssignedEvent& e) { second = e.slot; }));

	const auto server = MakeHost();
	auto client = MakeClient(server->GetBoundPort());
	ASSERT_TRUE(PumpUntil([&first] { return first.has_value(); })) << "the first client was told no seat";
	ASSERT_EQ(PlayerSlot::P1, *first);

	bool goodbye{false};
	subs.push_back(_hostEvents->AddListener([&goodbye](const ServerInDisconnectEvent&) { goodbye = true; }));

	client.reset();
	ASSERT_TRUE(PumpUntil([&goodbye] { return goodbye; })) << "host never got the client's goodbye";

	const auto next = std::make_unique<network::commands::ClientNode>("127.0.0.1", server->GetBoundPort(),
																				_secondClientEvents);
	ASSERT_TRUE(PumpUntil([&second] { return second.has_value(); })) << "the freed seat was never handed out";
	EXPECT_EQ(PlayerSlot::P1, *second) << "the seat of a client that quit is still held by its session";
}

// A client drives the keyboard half of the seat it was given, and only that one
TEST_F(NetworkTest, EachClientPutsOnlyItsOwnSeatOnTheWire)
{
	std::optional<PlayerSlot> firstSeat{};
	std::optional<PlayerSlot> secondSeat{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_clientEvents->AddListener([&firstSeat](const PlayerSlotAssignedEvent& e) { firstSeat = e.slot; }));
	subs.push_back(_secondClientEvents->AddListener(
			[&secondSeat](const PlayerSlotAssignedEvent& e) { secondSeat = e.slot; }));

	const auto server = MakeHost();
	const auto client = MakeClient(server->GetBoundPort());
	ASSERT_TRUE(PumpUntil([&firstSeat] { return firstSeat.has_value(); }));
	const auto secondClient = std::make_unique<network::commands::ClientNode>("127.0.0.1",
																				 server->GetBoundPort(),
																				 _secondClientEvents);
	ASSERT_TRUE(PumpUntil([&secondSeat] { return secondSeat.has_value(); }));

	bool firstSeatMoved{false};
	bool secondSeatMoved{false};
	bool strayArrived{false};
	subs.push_back(_hostEvents->AddListener(Key(InputChannel::RemoteP1),
											[&firstSeatMoved](const MoveUpEvent& e)
											{
												firstSeatMoved = e.isPressed;
											}));
	subs.push_back(_hostEvents->AddListener(Key(InputChannel::RemoteP2),
											[&secondSeatMoved](const MoveUpEvent& e)
											{
												secondSeatMoved = e.isPressed;
											}));
	subs.push_back(_hostEvents->AddListener(Key(InputChannel::RemoteP1),
											[&strayArrived](const MoveDownEvent&) { strayArrived = true; }));

	//NOTE: the stray goes first - the wire keeps its order, so once the press behind it has landed,
	//a press that never left is told apart from one still in flight
	_clientEvents->EmitEvent(Key(InputChannel::LocalP2), MoveDownEvent{.isPressed = true});
	_clientEvents->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = true});
	_secondClientEvents->EmitEvent(Key(InputChannel::LocalP2), MoveUpEvent{.isPressed = true});

	ASSERT_TRUE(PumpUntil([&firstSeatMoved, &secondSeatMoved] { return firstSeatMoved && secondSeatMoved; }));
	EXPECT_FALSE(strayArrived) << "a client sent the keyboard half of a seat it was never given";
}

TEST(SerializerTest, UnreadableFrameIsReportedNotSwallowed)
{
	const auto batch = network::Deserialize("not an archive at all");

	ASSERT_FALSE(batch.has_value());
	EXPECT_FALSE(batch.error().reason.empty());
}

TEST(SerializerTest, ABatchSurvivesTheRoundTrip)
{
	network::commands::CommandBatch sent;
	sent.commands.emplace_back(network::commands::Disconnect{.reason = DisconnectReason::GameOver});

	const std::string frame{network::SerializeFrame(sent)};
	ASSERT_GT(frame.size(), network::kFrameHeaderSize);
	//NOTE: FrameChannel trusts the length prefix - one disagreeing with the payload cuts every frame after it short
	EXPECT_EQ(network::DecodeFrameHeader(frame.data()), frame.size() - network::kFrameHeaderSize);

	const auto received = network::Deserialize(frame.substr(network::kFrameHeaderSize));

	ASSERT_TRUE(received.has_value());
	const auto& commands = received.value().commands;
	ASSERT_EQ(commands.size(), 1u);
	const auto* goodbye = std::get_if<network::commands::Disconnect>(&commands.front());
	ASSERT_NE(goodbye, nullptr);
	EXPECT_EQ(goodbye->reason, DisconnectReason::GameOver);
}

// Eleven local types collapse onto one StatisticsChange, so the discriminator table is all that keeps
// them apart. Every type here carries a different author - identical payloads would hide a crossed wire
TEST_F(NetworkTest, EveryStatisticsTypeKeepsItsOwnEventAcrossTheWire)
{
	const auto server = MakeHost();
	const auto client = MakeClient(server->GetBoundPort());
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	std::optional<StatisticsTankHitEvent> tankHit{};
	std::optional<TankDiedEvent> tankDied{};
	std::optional<BrickWallDiedEvent> brickDied{};
	std::optional<SteelWallDiedEvent> steelDied{};
	std::optional<StatisticsBonusPickupEvent> bonusPickup{};
	std::optional<StatisticsBonusDestroyedEvent> bonusDestroyed{};
	bool bonusExpired{false};

	std::vector<EventSubscription> subs{};
	subs.push_back(_clientEvents->AddListener([&tankHit](const StatisticsTankHitEvent& e) { tankHit = e; }));
	subs.push_back(_clientEvents->AddListener([&tankDied](const TankDiedEvent& e) { tankDied = e; }));
	subs.push_back(_clientEvents->AddListener([&brickDied](const BrickWallDiedEvent& e) { brickDied = e; }));
	subs.push_back(_clientEvents->AddListener([&steelDied](const SteelWallDiedEvent& e) { steelDied = e; }));
	subs.push_back(_clientEvents->AddListener(
			[&bonusPickup](const StatisticsBonusPickupEvent& e) { bonusPickup = e; }));
	subs.push_back(_clientEvents->AddListener(
			[&bonusDestroyed](const StatisticsBonusDestroyedEvent& e) { bonusDestroyed = e; }));
	subs.push_back(_clientEvents->AddListener(
			[&bonusExpired](const StatisticsBonusExpiredEvent&) { bonusExpired = true; }));

	_hostEvents->EmitEvent(StatisticsTankHitEvent{.who = Author::Player1, .author = Author::Enemy1});
	_hostEvents->EmitEvent(TankDiedEvent{.who = Author::Player2, .uuid = _uuid, .author = Author::Enemy2});
	_hostEvents->EmitEvent(BrickWallDiedEvent{.author = Author::Enemy3});
	_hostEvents->EmitEvent(SteelWallDiedEvent{.author = Author::Enemy4});
	_hostEvents->EmitEvent(StatisticsBonusPickupEvent{.author = Author::Player1});
	_hostEvents->EmitEvent(StatisticsBonusDestroyedEvent{.author = Author::Player2});
	_hostEvents->EmitEvent(StatisticsBonusExpiredEvent{});

	ASSERT_TRUE(PumpUntil([&]
	{
		return tankHit && tankDied && brickDied && steelDied && bonusPickup && bonusDestroyed && bonusExpired;
	}));

	EXPECT_EQ(Author::Player1, tankHit.value().who);
	EXPECT_EQ(Author::Enemy1, tankHit.value().author);
	EXPECT_EQ(Author::Player2, tankDied.value().who);
	EXPECT_EQ(_uuid, tankDied.value().uuid);
	EXPECT_EQ(Author::Enemy2, tankDied.value().author);
	EXPECT_EQ(Author::Enemy3, brickDied.value().author);
	EXPECT_EQ(Author::Enemy4, steelDied.value().author);
	EXPECT_EQ(Author::Player1, bonusPickup.value().author);
	EXPECT_EQ(Author::Player2, bonusDestroyed.value().author);
}
