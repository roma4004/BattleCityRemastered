#include "geometry/Point.h"
#include "components/EventSystem.h"
#include "components/MatchSettings.h"
#include "components/events/SpawnEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/ServerConsoleEvents.h"
#include "components/events/StatisticsEvents.h"
#include "components/events/TimingEvents.h"
#include "components/WorldSnapshot.h"
#include "enums/Absence.h"
#include "enums/MatchRules.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/DespawnReason.h"
#include "enums/DisconnectReason.h"
#include "enums/GameState.h"
#include "enums/InputChannel.h"
#include "enums/ObstacleType.h"
#include "enums/PlayerSlot.h"
#include "enums/TankType.h"
#include "network/ClientNode.h"
#include "network/DatagramLink.h"
#include "network/Serializer.h"
#include "network/commands/CommandBatch.h"
#include "network/commands/Disconnect.h"
#include "network/commands/SlotAssignment.h"
#include "network/ServerNode.h"
#include "gtest/gtest.h"
#include "utils/Uuid.h"
#include <array>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/udp.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <optional>
#include <string>
#include <thread>
#include <variant>
#include <vector>
#include "utils/UuidUtils.h"
#include "TestUtils.h"//NOTE: PrintTo for the Point types

using namespace std::chrono_literals;

// a real server and client over loopback: the host emits a local event and the client is asked if it arrived
class NetworkTest : public testing::Test
{
protected:
	Uuid _uuid{UuidUtils::GetUuidFromString("01234567-89ab-cdef-0123-456789abcdef")};

	void SetUp() override {}

	void TearDown() override {}

	//NOTE: one bus per node - on a shared bus a listener fires off the local emit before anything
	//crosses the wire, and the test passes with no networking at all
	std::shared_ptr<EventSystem> _serverEvents{std::make_shared<EventSystem>()};
	std::shared_ptr<EventSystem> _clientEvents{std::make_shared<EventSystem>()};
	//NOTE: a seat is handed out per connection, so telling the two apart takes a second game process
	std::shared_ptr<EventSystem> _secondClientEvents{std::make_shared<EventSystem>()};
	//NOTE: the one that finds both seats taken
	std::shared_ptr<EventSystem> _thirdClientEvents{std::make_shared<EventSystem>()};
	double _deltaTimeOneFrame{1.0 / 60.0};

	//NOTE: stands in for MainLoop, in its order - nothing is received or sent without it
	void Pump() const
	{
		_serverEvents->EmitEvent(NetCommandUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		_clientEvents->EmitEvent(NetCommandUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		_secondClientEvents->EmitEvent(NetCommandUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		_thirdClientEvents->EmitEvent(NetCommandUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		_serverEvents->EmitEvent(PreTickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		_clientEvents->EmitEvent(PreTickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		_secondClientEvents->EmitEvent(PreTickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		_thirdClientEvents->EmitEvent(PreTickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		_serverEvents->EmitEvent(NetworkEndFrameEvent{});
		_clientEvents->EmitEvent(NetworkEndFrameEvent{});
		_secondClientEvents->EmitEvent(NetworkEndFrameEvent{});
		_thirdClientEvents->EmitEvent(NetworkEndFrameEvent{});
	}

	//NOTE: listeners fire from the drain Pump does, so on this thread - a captured variable is enough
	template<typename Predicate>
	[[nodiscard]] bool PumpUntil(Predicate predicate, const std::chrono::milliseconds timeout = kWaitTimeout) const
	{
		const auto deadline{std::chrono::steady_clock::now() + timeout};
		while (!predicate() && std::chrono::steady_clock::now() < deadline)
		{
			Pump();
			std::this_thread::sleep_for(1ms);
		}

		return predicate();
	}

	//NOTE: port 0 - the OS picks a free one, so a leftover socket cannot collide
	[[nodiscard]] std::unique_ptr<network::commands::ServerNode> MakeServer() const
	{
		return std::make_unique<network::commands::ServerNode>(network::ServerAddress{.port = 0}, _serverEvents);
	}

	[[nodiscard]] std::unique_ptr<network::commands::ClientNode> MakeClient(const uint16_t port) const
	{
		return std::make_unique<network::commands::ClientNode>(network::ServerAddress{.port = port}, _clientEvents);
	}

	[[nodiscard]] EventSubscription AnnounceReadyOnConnect() const
	{
		return _clientEvents->AddListener([this](const ClientConnectedToHostEvent&)
		{
			_clientEvents->EmitEvent(ClientOutReadyToPlayEvent{});
		});
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
				server = std::make_unique<network::commands::ServerNode>(network::ServerAddress{.port = port},
																		_serverEvents);
			}
			catch (const std::exception&)
			{
				return false;
			}

			return true;
		}, kRebindTimeout);
	}

	//NOTE: a bare link on one socket - a ClientNode would open a socket of its own for every connection
	[[nodiscard]] std::optional<PlayerSlot> Dial(boost::asio::ip::udp::socket& socket,
												 const std::uint32_t connectionId) const
	{
		network::DatagramLink link{connectionId, network::DatagramLink::Clock::now()};
		std::optional<PlayerSlot> seat{};
		std::ignore = PumpUntil([&link, &socket, &seat]
		{
			const auto now{network::DatagramLink::Clock::now()};
			for (const std::string& datagram: link.TakeDatagrams(now))
			{
				socket.send(boost::asio::buffer(datagram));
			}

			std::array<char, network::DatagramLink::kMaxDatagramSize> buffer{};
			boost::system::error_code ec;
			const std::size_t size{socket.receive(boost::asio::buffer(buffer), 0, ec)};
			const auto arrivals{ec ? std::nullopt : link.Receive(std::string_view{buffer.data(), size}, now)};
			for (const std::string& message: arrivals ? arrivals->messages : std::vector<std::string>{})
			{
				for (const auto& command: network::Deserialize(message)->commands)
				{
					if (const auto* assignment{std::get_if<network::commands::SlotAssignment>(&command)})
					{
						seat = assignment->slot;
					}
				}
			}

			return seat.has_value();
		});

		return seat;
	}

	static constexpr auto kWaitTimeout{5s};
	//NOTE: short on purpose - the client gives up after ~10s, and a slow re-bind eats that window
	static constexpr auto kRebindTimeout{1500ms};
};


// a tank's position crosses the wire and arrives keyed on the same uuid
TEST_F(NetworkTest, PosEventReplication)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	constexpr FPoint posOrigin{.x = 42.0, .y = 42.0};
	constexpr auto directionOrigin{Direction::UP};

	std::optional<PosChangedEvent> received{};
	auto posSub{_clientEvents->AddListener(Key(_uuid),
										   [&received](const PosChangedEvent& event) { received = event; })};

	_serverEvents->EmitEvent(
			PosChangedEvent{.pos = posOrigin, .dir = directionOrigin, .uuid = _uuid});

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); }));
	const auto& event{received.value()};
	EXPECT_EQ(posOrigin, event.pos);
	EXPECT_EQ(directionOrigin, event.dir);
}

// so does a shot, with the direction it was fired in
TEST_F(NetworkTest, ShotEventReplication)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	constexpr auto direction{Direction::UP};
	constexpr auto who{Author::Player1};

	std::optional<TankShotEvent> received{};
	auto shotSub{_clientEvents->AddListener(Key(who),
											[&received](const TankShotEvent& event) { received = event; })};

	_serverEvents->EmitEvent(TankShotEvent{.who = who, .dir = direction, .bulletUuid = _uuid});

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); }));
	const auto& event{received.value()};
	EXPECT_EQ(direction, event.dir);
	EXPECT_EQ(_uuid, event.bulletUuid);
}

// and a health change
TEST_F(NetworkTest, HealthEventReplication)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	constexpr int healthOrigin{42};

	std::optional<int> received{};
	auto healthSub{_clientEvents->AddListener(Key(_uuid),
											  [&received](const HealthChangedEvent& event)
											  {
												  received = event.health;
											  })};

	_serverEvents->EmitEvent(HealthChangedEvent{.health = healthOrigin, .uuid = _uuid});

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); }));
	EXPECT_EQ(healthOrigin, *received);
}

//NOTE: two despawns - a destroyed bullet and a picked-up bonus travel the same command
TEST_F(NetworkTest, DespawnEventReplication)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	std::vector<DespawnedEvent> received{};
	auto despawnSub{_clientEvents->AddListener(Key(_uuid),
											   [&received](const DespawnedEvent& event)
											   {
												   received.push_back(event);
											   })};

	_serverEvents->EmitEvent(DespawnedEvent{.uuid = _uuid, .reason = DespawnReason::Destroyed});
	_serverEvents->EmitEvent(DespawnedEvent{.uuid = _uuid, .reason = DespawnReason::PickedUp});

	ASSERT_TRUE(PumpUntil([&received] { return received.size() == 2u; }));
	EXPECT_EQ(_uuid, received[0].uuid);
	EXPECT_EQ(DespawnReason::Destroyed, received[0].reason);
	EXPECT_EQ(DespawnReason::PickedUp, received[1].reason);
}

// a counter event the host raised reaches the client's own statistics
TEST_F(NetworkTest, StatisticsEventReplication)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	std::optional<StatisticsBulletHitEvent> received{};
	auto statsSub{_clientEvents->AddListener(
			[&received](const StatisticsBulletHitEvent& event) { received = event; })};

	_serverEvents->EmitEvent(StatisticsBulletHitEvent{.author = Author::Enemy1});

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); }));
	EXPECT_EQ(Author::Enemy1, received.value().author);
}

// one of the four things a client may ask for: the pause travels the other way and stops the host
TEST_F(NetworkTest, PauseRequestFromClientPausesTheServer)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	bool isServerPaused{};
	auto pauseSub{_serverEvents->AddListener(
			[&isServerPaused](const SetPauseEvent& event) { isServerPaused = event.isPaused; })};

	_clientEvents->EmitEvent(PauseRequestedEvent{.isPaused = true});

	EXPECT_TRUE(PumpUntil([&isServerPaused] { return isServerPaused; }));

	//NOTE: the state the client asked for, not a toggle - a second client must not undo the first one's pause
	_clientEvents->EmitEvent(PauseRequestedEvent{.isPaused = false});

	EXPECT_TRUE(PumpUntil([&isServerPaused] { return !isServerPaused; }));
}

// a bonus appearing on the host appears on the client with its place and kind
TEST_F(NetworkTest, BonusSpawnEventReplication)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	constexpr FPoint pos{.x = 42.0, .y = 42.0};
	constexpr auto type{BonusType::Timer};

	std::optional<BonusSpawnedEvent> received{};
	auto bonusSpawnSub{_clientEvents->AddListener(
			[&received](const BonusSpawnedEvent& event) { received = event; })};

	_serverEvents->EmitEvent(BonusSpawnedEvent{.pos = pos, .type = type, .uuid = _uuid, .isSuper = true});

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); }));
	const auto& event{received.value()};
	EXPECT_EQ(pos, event.pos);
	EXPECT_EQ(type, event.type);
	EXPECT_EQ(_uuid, event.uuid);
	EXPECT_TRUE(event.isSuper);
}

//NOTE: what makes the bonus real on the client - its own burst only draws
TEST_F(NetworkTest, BonusSpawnCompleteEventReplication)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	std::optional<Uuid> received{};
	auto bonusSpawnCompleteSub{_clientEvents->AddListener(
			[&received](const BonusSpawnCompletedEvent& event) { received = event.uuid; })};

	_serverEvents->EmitEvent(BonusSpawnCompletedEvent{.uuid = _uuid});

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); }));
	EXPECT_EQ(_uuid, *received);
}

// an effect switching on for a seat arrives keyed on that seat
TEST_F(NetworkTest, BonusStatusEventReplication)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	constexpr auto authorOrigin{Author::Player1};
	constexpr bool isActiveOrigin{true};

	std::optional<bool> received{};
	auto bonusStatusSub{_clientEvents->AddListener(Key(authorOrigin),
												   [&received](const BonusHelmetAppliedEvent& event)
												   {
													   received = event.isActive;
												   })};

	_serverEvents->EmitEvent(BonusHelmetAppliedEvent{.author = authorOrigin, .isActive = isActiveOrigin});

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); }));
	EXPECT_EQ(isActiveOrigin, *received);
}

//NOTE: the tier travels as a result, on its own command - the client sets it instead of replaying the
//upgrade formula, exactly as it does with health
TEST_F(NetworkTest, TierEventReplication)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	constexpr unsigned short tierOrigin{4u};

	std::optional<unsigned short> received{};
	auto tierSub{_clientEvents->AddListener(Key(_uuid),
											[&received](const TierChangedEvent& event) { received = event.tier; })};

	_serverEvents->EmitEvent(TierChangedEvent{.tier = tierOrigin, .uuid = _uuid});

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); }));
	EXPECT_EQ(tierOrigin, *received);
}

//NOTE: no payload of its own - under test is that its alternative reaches the right seat
TEST_F(NetworkTest, BonusShipStatusEventReplication)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	constexpr auto authorOrigin{Author::Player1};

	bool received{};
	auto bonusShipSub{_clientEvents->AddListener(Key(authorOrigin),
												 [&received](const BonusShipAppliedEvent&) { received = true; })};

	_serverEvents->EmitEvent(BonusShipAppliedEvent{.author = authorOrigin});

	EXPECT_TRUE(PumpUntil([&received] { return received; }));
}

// one obstacle placed by the host is placed on the client too
TEST_F(NetworkTest, ObstacleSpawnEventReplication)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	constexpr auto obstacleType{ObstacleType::Brick};
	constexpr FPoint posOrigin{.x = 42.0, .y = 43.0};

	std::optional<ObstacleSpawnedEvent> received{};
	auto obstacleSpawnSub{_clientEvents->AddListener(
			[&received](const ObstacleSpawnedEvent& event) { received = event; })};

	_serverEvents->EmitEvent(ObstacleSpawnedEvent{.pos = posOrigin, .type = obstacleType, .uuid = _uuid});

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); }));
	const auto& event{received.value()};
	EXPECT_EQ(posOrigin, event.pos);
	EXPECT_EQ(obstacleType, event.type);
	EXPECT_EQ(_uuid, event.uuid);
}

//NOTE: a whole map in one batch - one frame, and every command has to come out of it in order
TEST_F(NetworkTest, MassiveObstacleSpawnEventReplication)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	constexpr auto obstacleType{ObstacleType::Brick};
	constexpr unsigned short itemsInMassiveTest{10000u};

	std::vector<FPoint> sent{};
	sent.reserve(itemsInMassiveTest);
	for (size_t i = 0u; i < itemsInMassiveTest; ++i)
	{
		const auto value{static_cast<double>(i)};
		sent.emplace_back(FPoint{.x = value, .y = value + 1.0});
	}

	std::vector<ObstacleSpawnedEvent> received{};
	received.reserve(itemsInMassiveTest);
	auto massiveObstacleSub{_clientEvents->AddListener(
			[&received](const ObstacleSpawnedEvent& event) { received.push_back(event); })};

	for (const auto& pos: sent)
	{
		_serverEvents->EmitEvent(ObstacleSpawnedEvent{.pos = pos, .type = obstacleType, .uuid = _uuid});
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

// every tank type respawns on the client where the host put it
TEST_F(NetworkTest, RespawnTankEventReplication)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
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
	auto respawnTankSub{_clientEvents->AddListener(
			[&received](const TankRespawnedEvent& event) { received.push_back(event); })};

	for (const auto tankType: tankTypes)
	{
		_serverEvents->EmitEvent(TankRespawnedEvent{.type = tankType,
													.model = TankModel::Basic,
													.uuid = _uuid,
													.pos = posOrigin});
	}

	ASSERT_TRUE(PumpUntil([&received] { return received.size() == expectedCount; }));
	for (size_t i = 0u; i < tankTypes.size(); ++i)
	{
		EXPECT_EQ(tankTypes[i], received[i].type);
		EXPECT_EQ(_uuid, received[i].uuid);
		EXPECT_EQ(posOrigin, received[i].pos);
	}
}

//NOTE: the link and what rides it - a server still thinking the old client plays reconnects nothing
TEST_F(NetworkTest, ClientReconnectsAfterEstablishedLinkDrops)
{
	auto server{MakeServer()};
	const uint16_t port{server->GetBoundPort()};
	const auto client{MakeClient(port)};

	int readySignals{};
	int linksUp{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_serverEvents->AddListener(
			[&readySignals](const ServerInClientReadyToStartGameEvent&) { ++readySignals; }));
	subs.push_back(_clientEvents->AddListener([&linksUp](const ClientConnectedToHostEvent&) { ++linksUp; }));
	subs.push_back(AnnounceReadyOnConnect());

	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));
	ASSERT_TRUE(PumpUntil([&readySignals] { return readySignals == 1; })) << "the server never got the first ready";

	//NOTE: Abort, not just reset - this test is about a link that dies without a goodbye
	server->Abort();
	server.reset();

	ASSERT_TRUE(PumpUntil([&client] { return !client->IsConnected(); }))
			<< "client never noticed the link dropped";

	if (!ReboundHost(server, port))
	{
		GTEST_SKIP() << "port " << port << " still held by the OS - nothing to test against";
	}

	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }, 10s))
			<< "client did not reconnect after the server came back";

	//NOTE: pumped, not read straight off - IsConnected flips before the event behind it is drained
	EXPECT_TRUE(PumpUntil([&linksUp] { return linksUp == 2; }))
			<< "client did not tell its own side the link was back";

	EXPECT_TRUE(PumpUntil([&readySignals] { return readySignals == 2; }))
			<< "reconnected client never asked the server to start the match again";
}

//NOTE: the other half of ClientQuitTellsHostWhy - here the server outlives the loss and has to take the
//next client on the same acceptor
TEST_F(NetworkTest, TheServerLearnsTheClientDroppedWithoutSayingGoodbye)
{
	const auto server{MakeServer()};
	const uint16_t port{server->GetBoundPort()};
	auto client{MakeClient(port)};

	bool clientLost{};
	int readySignals{};
	std::optional<DisconnectReason> announced{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_serverEvents->AddListener([&clientLost](const ServerClientLostEvent&) { clientLost = true; }));
	subs.push_back(_serverEvents->AddListener(
			[&readySignals](const ServerInClientReadyToStartGameEvent&) { ++readySignals; }));
	subs.push_back(_serverEvents->AddListener(
			[&announced](const ServerInDisconnectEvent& event) { announced = event.reason; }));
	subs.push_back(AnnounceReadyOnConnect());

	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));
	ASSERT_TRUE(PumpUntil([&readySignals] { return readySignals == 1; }));

	client->Abort();
	client.reset();

	ASSERT_TRUE(PumpUntil([&clientLost] { return clientLost; }))
			<< "the server never noticed the client stopped answering";
	EXPECT_FALSE(announced.has_value()) << "a dropped link reported itself as an announced leave";

	const auto secondClient{MakeClient(port)};
	ASSERT_TRUE(PumpUntil([&secondClient] { return secondClient->IsConnected(); }))
			<< "the server stopped accepting after losing the first client";
	EXPECT_TRUE(PumpUntil([&readySignals] { return readySignals == 2; }))
			<< "host never got a ready from the client that replaced the lost one";
}

// a host that shuts down says why, and the client keeps trying to come back
TEST_F(NetworkTest, HostShutdownTellsClientWhyAndKeepsTheReconnect)
{
	auto server{MakeServer()};
	const uint16_t port{server->GetBoundPort()};
	const auto client{MakeClient(port)};

	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	std::optional<DisconnectReason> received{};
	bool gaveUp{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_clientEvents->AddListener(
			[&received](const ClientInDisconnectEvent& event) { received = event.reason; }));
	subs.push_back(_clientEvents->AddListener([&gaveUp](const ClientReconnectAbandonedEvent&) { gaveUp = true; }));

	server.reset();//NOTE: the goodbye goes out from inside the destructor before the socket closes

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); })) << "client never got the server's goodbye";
	EXPECT_EQ(DisconnectReason::HostShutdown, *received);

	if (!ReboundHost(server, port))
	{
		GTEST_SKIP() << "port " << port << " still held by the OS - nothing to test against";
	}

	EXPECT_TRUE(PumpUntil([&client] { return client->IsConnected(); }, 10s))
			<< "client did not dial back the server that only announced a restart";
	EXPECT_FALSE(gaveUp) << "client gave up on a host that was restarting the same mode";
}

// a client that quits says why as well, so the seat is freed rather than timed out
TEST_F(NetworkTest, ClientQuitTellsHostWhy)
{
	const auto server{MakeServer()};
	auto client{MakeClient(server->GetBoundPort())};

	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	std::optional<DisconnectReason> received{};
	bool clientLost{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_serverEvents->AddListener(
			[&received](const ServerInDisconnectEvent& event) { received = event.reason; }));
	subs.push_back(_serverEvents->AddListener([&clientLost](const ServerClientLostEvent&) { clientLost = true; }));

	client.reset();

	ASSERT_TRUE(PumpUntil([&received] { return received.has_value(); })) << "host never got the client's goodbye";
	EXPECT_EQ(DisconnectReason::PlayerQuit, *received);
	EXPECT_FALSE(clientLost) << "the silence after the goodbye was reported as a second, silent drop";
}

//TODO: other bonus effect replication test after write this replication
// TEST_F(NetworkTest, bonusKind...EventReplication) {

// Three effects ride BonusStatus and bonusType is all that tells them apart, so a swapped case arrives
// as the wrong effect on the wrong seat in silence. The ship lands keyed on its seat, the tank broadcasts
TEST_F(NetworkTest, ShipAndTankEffectsKeepTheirOwnEventAcrossTheWire)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
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

	_serverEvents->EmitEvent(BonusShipAppliedEvent{.author = shipSeat});
	_serverEvents->EmitEvent(BonusTankAppliedEvent{.author = tankSeat});

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

	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([&first] { return first.has_value(); })) << "the first client was told no seat";
	//NOTE: dialled only once the first seat is out, so the accept order is the test's, not the OS's
	const auto secondClient{std::make_unique<network::commands::ClientNode>(
			network::ServerAddress{.port = server->GetBoundPort()}, _secondClientEvents)};
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

	const auto server{MakeServer()};
	auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([&first] { return first.has_value(); })) << "the first client was told no seat";
	ASSERT_EQ(PlayerSlot::P1, *first);

	bool goodbye{};
	subs.push_back(_serverEvents->AddListener([&goodbye](const ServerInDisconnectEvent&) { goodbye = true; }));

	client.reset();
	ASSERT_TRUE(PumpUntil([&goodbye] { return goodbye; })) << "host never got the client's goodbye";

	const auto next{std::make_unique<network::commands::ClientNode>(
			network::ServerAddress{.port = server->GetBoundPort()}, _secondClientEvents)};
	ASSERT_TRUE(PumpUntil([&second] { return second.has_value(); })) << "the freed seat was never handed out";
	EXPECT_EQ(PlayerSlot::P1, *second) << "the seat of a client that quit is still held by its session";
}

// two clients on one machine: each tags its presses with its own seat, so neither drives the other's tank
TEST_F(NetworkTest, EachClientPutsOnlyItsOwnSeatOnTheWire)
{
	std::optional<PlayerSlot> firstSeat{};
	std::optional<PlayerSlot> secondSeat{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_clientEvents->AddListener([&firstSeat](const PlayerSlotAssignedEvent& e) { firstSeat = e.slot; }));
	subs.push_back(_secondClientEvents->AddListener(
			[&secondSeat](const PlayerSlotAssignedEvent& e) { secondSeat = e.slot; }));

	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([&firstSeat] { return firstSeat.has_value(); }));
	const auto secondClient{std::make_unique<network::commands::ClientNode>(
			network::ServerAddress{.port = server->GetBoundPort()}, _secondClientEvents)};
	ASSERT_TRUE(PumpUntil([&secondSeat] { return secondSeat.has_value(); }));

	bool firstSeatMoved{};
	bool secondSeatMoved{};
	bool strayArrived{};
	subs.push_back(_serverEvents->AddListener(Key(InputChannel::RemoteP1),
											[&firstSeatMoved](const MoveUpEvent& e)
											{
												firstSeatMoved = e.isPressed;
											}));
	subs.push_back(_serverEvents->AddListener(Key(InputChannel::RemoteP2),
											[&secondSeatMoved](const MoveUpEvent& e)
											{
												secondSeatMoved = e.isPressed;
											}));
	subs.push_back(_serverEvents->AddListener(Key(InputChannel::RemoteP1),
											[&strayArrived](const MoveDownEvent&) { strayArrived = true; }));

	//NOTE: the stray goes first - the wire keeps its order, so once the press behind it has landed,
	//a press that never left is told apart from one still in flight
	_clientEvents->EmitEvent(Key(InputChannel::LocalP2), MoveDownEvent{.isPressed = true});
	_clientEvents->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = true});
	_secondClientEvents->EmitEvent(Key(InputChannel::LocalP2), MoveUpEvent{.isPressed = true});

	ASSERT_TRUE(PumpUntil([&firstSeatMoved, &secondSeatMoved] { return firstSeatMoved && secondSeatMoved; }));
	EXPECT_FALSE(strayArrived) << "a client sent the keyboard half of a seat it was never given";
}

//NOTE: an unanswered hello reads as a host that is not there - a third player would burn its retries on
//one that is merely busy
TEST_F(NetworkTest, AThirdClientIsToldTheSeatsAreTaken)
{
	std::optional<PlayerSlot> firstSeat{};
	std::optional<PlayerSlot> secondSeat{};
	std::optional<DisconnectReason> refusal{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_clientEvents->AddListener([&firstSeat](const PlayerSlotAssignedEvent& e) { firstSeat = e.slot; }));
	subs.push_back(_secondClientEvents->AddListener(
			[&secondSeat](const PlayerSlotAssignedEvent& e) { secondSeat = e.slot; }));
	subs.push_back(_thirdClientEvents->AddListener(
			[&refusal](const ClientInDisconnectEvent& e) { refusal = e.reason; }));

	const auto server{MakeServer()};
	const uint16_t port{server->GetBoundPort()};
	const auto client{MakeClient(port)};
	const auto secondClient{std::make_unique<network::commands::ClientNode>(network::ServerAddress{.port = port},
																					_secondClientEvents)};
	ASSERT_TRUE(PumpUntil([&firstSeat, &secondSeat] { return firstSeat && secondSeat; }));

	const auto thirdClient{std::make_unique<network::commands::ClientNode>(network::ServerAddress{.port = port},
																				   _thirdClientEvents)};

	ASSERT_TRUE(PumpUntil([&refusal] { return refusal.has_value(); })) << "the third client heard nothing";
	EXPECT_EQ(*refusal, DisconnectReason::ServerFull);
}

// a three-seat host seats a third client and tells it the match it was started for
TEST_F(NetworkTest, AThreeSeatServerSeatsAThirdClientAndTellsItTheMatch)
{
	std::optional<PlayerSlotAssignedEvent> third{};
	const EventSubscription thirdSub{_thirdClientEvents->AddListener(
			[&third](const PlayerSlotAssignedEvent& e) { third = e; })};

	const MatchSettings match{.rules = MatchRules::FreeForAll,
							  .seats = 3u,
							  .map = "level2",
							  .enemiesAtOnce = 1u,
							  .bots = 2u,
							  .isStartingAtOnce = true};
	const auto server{std::make_unique<network::commands::ServerNode>(network::ServerAddress{.port = 0},
																	  _serverEvents, match)};
	const uint16_t port{server->GetBoundPort()};
	const auto client{MakeClient(port)};
	const auto secondClient{std::make_unique<network::commands::ClientNode>(network::ServerAddress{.port = port},
																			_secondClientEvents)};
	const auto thirdClient{std::make_unique<network::commands::ClientNode>(network::ServerAddress{.port = port},
																		   _thirdClientEvents)};

	ASSERT_TRUE(PumpUntil([&third] { return third.has_value(); })) << "the third client was told no seat";
	EXPECT_EQ(third->slot, PlayerSlot::P3);
	EXPECT_EQ(third->match, match);
}

// A ready landing in a running match is owed the field, and the snapshot goes out in place of that frame -
// sent beside it, the frame's spawns would land a second time on top
TEST_F(NetworkTest, AReadyInARunningMatchIsAnsweredWithTheFieldInsteadOfTheFrame)
{
	const Uuid standing{UuidUtils::GetRandomUuid()};
	const Uuid inTheFrame{UuidUtils::GetRandomUuid()};
	std::optional<std::vector<ObstacleSnapshot>> field{};
	std::vector<Uuid> spawned{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_serverEvents->AddListener([standing](const WorldSnapshotRequestedEvent& event)
	{
		event.snapshot.phase = GameState::Playing;
		event.snapshot.obstacles.push_back(
				ObstacleSnapshot{.pos = {}, .type = ObstacleType::Steel, .uuid = standing});
	}));
	subs.push_back(_serverEvents->AddListener([this, inTheFrame](const ServerInClientReadyToStartGameEvent&)
	{
		_serverEvents->EmitEvent(ObstacleSpawnedEvent{.pos = {}, .type = ObstacleType::Brick, .uuid = inTheFrame});
	}));
	subs.push_back(_clientEvents->AddListener([&field](const WorldSnapshotReceivedEvent& event)
	{
		field = event.snapshot.obstacles;
	}));
	subs.push_back(_clientEvents->AddListener([&spawned](const ObstacleSpawnedEvent& event)
	{
		spawned.push_back(event.uuid);
	}));
	subs.push_back(AnnounceReadyOnConnect());

	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};

	ASSERT_TRUE(PumpUntil([&field] { return field.has_value(); })) << "the late joiner never got the field";
	ASSERT_EQ(field->size(), 1u);
	EXPECT_EQ(field->front().uuid, standing);

	const Uuid afterwards{UuidUtils::GetRandomUuid()};
	_serverEvents->EmitEvent(ObstacleSpawnedEvent{.pos = {}, .type = ObstacleType::Brick, .uuid = afterwards});
	ASSERT_TRUE(PumpUntil([&spawned] { return !spawned.empty(); })) << "frames stopped after the snapshot";
	EXPECT_EQ(spawned, std::vector{afterwards}) << "the frame the snapshot replaced reached the client as well";
}

// A lobby has no field to catch up with - the ready is answered by the frames, as it always was
TEST_F(NetworkTest, AReadyInTheLobbyIsAnsweredWithFramesNotASnapshot)
{
	const Uuid inTheFrame{UuidUtils::GetRandomUuid()};
	bool isSnapshotReceived{};
	std::optional<Uuid> spawned{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_serverEvents->AddListener([](const WorldSnapshotRequestedEvent& event)
	{
		event.snapshot.phase = GameState::Lobby;
	}));
	subs.push_back(_serverEvents->AddListener([this, inTheFrame](const ServerInClientReadyToStartGameEvent&)
	{
		_serverEvents->EmitEvent(ObstacleSpawnedEvent{.pos = {}, .type = ObstacleType::Brick, .uuid = inTheFrame});
	}));
	subs.push_back(_clientEvents->AddListener([&isSnapshotReceived](const WorldSnapshotReceivedEvent&)
	{
		isSnapshotReceived = true;
	}));
	subs.push_back(_clientEvents->AddListener([&spawned](const ObstacleSpawnedEvent& event) { spawned = event.uuid; }));
	subs.push_back(AnnounceReadyOnConnect());

	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};

	ASSERT_TRUE(PumpUntil([&spawned] { return spawned.has_value(); })) << "the frame of the ready never arrived";
	EXPECT_EQ(*spawned, inTheFrame);
	EXPECT_FALSE(isSnapshotReceived);
}

// A lobby ready has no field to catch up with, but the debt it leaves has to outlive the lobby - the
// first one in is playing on a field nobody would ever send him otherwise
TEST_F(NetworkTest, AReadyOwedFromTheLobbyIsPaidOnceTheMatchStarts)
{
	const Uuid standing{UuidUtils::GetRandomUuid()};
	const Uuid inTheFrame{UuidUtils::GetRandomUuid()};
	GameState phase{GameState::Lobby};
	std::optional<std::vector<ObstacleSnapshot>> field{};
	std::optional<Uuid> spawned{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_serverEvents->AddListener([&phase, standing](const WorldSnapshotRequestedEvent& event)
	{
		event.snapshot.phase = phase;
		event.snapshot.obstacles.push_back(
				ObstacleSnapshot{.pos = {}, .type = ObstacleType::Steel, .uuid = standing});
	}));
	subs.push_back(_serverEvents->AddListener([this, inTheFrame](const ServerInClientReadyToStartGameEvent&)
	{
		_serverEvents->EmitEvent(ObstacleSpawnedEvent{.pos = {}, .type = ObstacleType::Brick, .uuid = inTheFrame});
	}));
	subs.push_back(_clientEvents->AddListener([&field](const WorldSnapshotReceivedEvent& event)
	{
		field = event.snapshot.obstacles;
	}));
	subs.push_back(_clientEvents->AddListener([&spawned](const ObstacleSpawnedEvent& event) { spawned = event.uuid; }));
	subs.push_back(AnnounceReadyOnConnect());

	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};

	ASSERT_TRUE(PumpUntil([&spawned] { return spawned.has_value(); })) << "the frame of the ready never arrived";
	ASSERT_FALSE(field.has_value()) << "the control failed - a lobby has no field to send";

	phase = GameState::Playing;

	EXPECT_TRUE(PumpUntil([&field] { return field.has_value(); })) << "the debt the lobby left was dropped";
}

// The match goes on without a lost player, so a key it held would keep its tank driving into a wall
TEST_F(NetworkTest, ALostClientLetsGoOfTheKeysOfItsSeat)
{
	std::optional<PlayerSlot> seat{};
	std::optional<bool> isUpHeld{};
	bool isClientLost{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_clientEvents->AddListener([&seat](const PlayerSlotAssignedEvent& event) { seat = event.slot; }));
	subs.push_back(_serverEvents->AddListener(Key(InputChannel::RemoteP1),
											[&isUpHeld](const MoveUpEvent& event) { isUpHeld = event.isPressed; }));
	subs.push_back(_serverEvents->AddListener([&isClientLost](const ServerClientLostEvent&) { isClientLost = true; }));

	const auto server{MakeServer()};
	auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([&seat] { return seat.has_value(); }));
	ASSERT_EQ(*seat, PlayerSlot::P1);

	_clientEvents->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = true});
	ASSERT_TRUE(PumpUntil([&isUpHeld] { return isUpHeld.value_or(false); }));

	client->Abort();
	client.reset();

	ASSERT_TRUE(PumpUntil([&isClientLost] { return isClientLost; }));
	EXPECT_FALSE(isUpHeld.value_or(true)) << "the seat's tank still drives on a key nobody holds";
}

// A client whose link timed out dials again from the same socket before the host noticed - it gets its seat
// back instead of being told the match is full of the connection it replaced
TEST_F(NetworkTest, AClientDialingAgainFromTheSameAddressTakesItsSeatBack)
{
	std::optional<PlayerSlot> lost{};
	std::optional<PlayerSlot> secondSeat{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_serverEvents->AddListener([&lost](const ServerClientLostEvent& event) { lost = event.slot; }));
	subs.push_back(_secondClientEvents->AddListener(
			[&secondSeat](const PlayerSlotAssignedEvent& event) { secondSeat = event.slot; }));

	const auto server{MakeServer()};
	boost::asio::io_context ioContext;
	boost::asio::ip::udp::socket socket{ioContext, boost::asio::ip::udp::v4()};
	socket.connect({boost::asio::ip::make_address("127.0.0.1"), server->GetBoundPort()});
	socket.non_blocking(true);

	ASSERT_EQ(Dial(socket, 1u), PlayerSlot::P1);
	const auto secondClient{std::make_unique<network::commands::ClientNode>(
			network::ServerAddress{.port = server->GetBoundPort()}, _secondClientEvents)};
	ASSERT_TRUE(PumpUntil([&secondSeat] { return secondSeat.has_value(); }));

	EXPECT_EQ(Dial(socket, 2u), PlayerSlot::P1) << "the new connection was refused a seat its own old one held";
	ASSERT_TRUE(PumpUntil([&lost] { return lost.has_value(); })) << "the replaced connection was never let go";
	EXPECT_EQ(*lost, PlayerSlot::P1);
}

// a frame that is not an archive comes back as an error with a reason, not as an empty batch
TEST(SerializerTest, UnreadableFrameIsReportedNotSwallowed)
{
	const auto batch{network::Deserialize("not an archive at all")};

	ASSERT_FALSE(batch.has_value());
	EXPECT_FALSE(batch.error().reason.empty());
}

// and a batch written and read back holds the same command with the same fields
TEST(SerializerTest, ABatchSurvivesTheRoundTrip)
{
	network::commands::CommandBatch sent;
	sent.commands.emplace_back(network::commands::Disconnect{.reason = DisconnectReason::GameOver});

	const auto received{network::Deserialize(network::Serialize(sent))};

	ASSERT_TRUE(received.has_value());
	const auto& commands{received.value().commands};
	ASSERT_EQ(commands.size(), 1u);
	const auto* goodbye{std::get_if<network::commands::Disconnect>(&commands.front())};
	ASSERT_NE(goodbye, nullptr);
	EXPECT_EQ(goodbye->reason, DisconnectReason::GameOver);
}

// Eleven local types collapse onto one StatisticsChange, so the discriminator table is all that keeps
// them apart. Every type here carries a different author - identical payloads would hide a crossed wire
TEST_F(NetworkTest, EveryStatisticsTypeKeepsItsOwnEventAcrossTheWire)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	std::optional<StatisticsTankHitEvent> tankHit{};
	std::optional<TankDiedEvent> tankDied{};
	std::optional<BrickWallDiedEvent> brickDied{};
	std::optional<SteelWallDiedEvent> steelDied{};
	std::optional<StatisticsBonusPickupEvent> bonusPickup{};
	std::optional<StatisticsBonusDestroyedEvent> bonusDestroyed{};
	bool bonusExpired{};

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

	_serverEvents->EmitEvent(StatisticsTankHitEvent{.who = Author::Player1, .author = Author::Enemy1});
	_serverEvents->EmitEvent(TankDiedEvent{.who = Author::Player2, .uuid = _uuid, .author = Author::Enemy2});
	_serverEvents->EmitEvent(BrickWallDiedEvent{.author = Author::Enemy3});
	_serverEvents->EmitEvent(SteelWallDiedEvent{.author = Author::Enemy4});
	_serverEvents->EmitEvent(StatisticsBonusPickupEvent{.author = Author::Player1});
	_serverEvents->EmitEvent(StatisticsBonusDestroyedEvent{.author = Author::Player2});
	_serverEvents->EmitEvent(StatisticsBonusExpiredEvent{});

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

//NOTE: a kicked client that dialled straight back would sit in the seat it was sent away from
TEST_F(NetworkTest, AKickedClientIsToldWhyAndDoesNotDialBack)
{
	std::optional<DisconnectReason> reason{};
	bool isAbandoned{};
	bool isSeatFreed{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_clientEvents->AddListener([&reason](const ClientInDisconnectEvent& e) { reason = e.reason; }));
	subs.push_back(_clientEvents->AddListener(
			[&isAbandoned](const ClientReconnectAbandonedEvent&) { isAbandoned = true; }));
	subs.push_back(_serverEvents->AddListener([&isSeatFreed](const ServerClientLostEvent&) { isSeatFreed = true; }));

	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	_serverEvents->EmitEvent(ServerKickRequestedEvent{.slot = PlayerSlot::P1});

	ASSERT_TRUE(PumpUntil([&reason, &isAbandoned, &isSeatFreed] { return reason && isAbandoned && isSeatFreed; }))
			<< "told why: " << reason.has_value() << ", gave up: " << isAbandoned << ", seat freed: " << isSeatFreed;
	EXPECT_EQ(*reason, DisconnectReason::Kicked);
}

// a closed server turns a hello away and seats it only once it opens again
TEST_F(NetworkTest, AClosedServerSeatsNoClientUntilItOpens)
{
	const auto server{MakeServer()};
	_serverEvents->EmitEvent(ServerAcceptingChangedEvent{.isAccepting = false});

	const auto client{MakeClient(server->GetBoundPort())};
	EXPECT_FALSE(PumpUntil([&client] { return client->IsConnected(); }, 1s)) << "a closed server seated a client";

	_serverEvents->EmitEvent(ServerAcceptingChangedEvent{.isAccepting = true});
	EXPECT_TRUE(PumpUntil([&client] { return client->IsConnected(); })) << "the reopened server never seated it";
}

// a hello the server turns away is no connection - the client must not report a seat it never got
TEST_F(NetworkTest, ATurnedAwayHelloIsNoConnection)
{
	const auto server{MakeServer()};
	_serverEvents->EmitEvent(ServerAcceptingChangedEvent{.isAccepting = false});

	int linksUp{};
	std::optional<DisconnectReason> refusal{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_clientEvents->AddListener([&linksUp](const ClientConnectedToHostEvent&) { ++linksUp; }));
	subs.push_back(_clientEvents->AddListener(
			[&refusal](const ClientInDisconnectEvent& event) { refusal = event.reason; }));

	const auto client{MakeClient(server->GetBoundPort())};

	ASSERT_TRUE(PumpUntil([&refusal] { return refusal.has_value(); })) << "the closed server never turned it away";
	EXPECT_EQ(DisconnectReason::ServerFull, *refusal);
	EXPECT_EQ(0, linksUp) << "a turned-away client told its own side the link was up";
	EXPECT_FALSE(client->IsConnected()) << "a turned-away client counts itself seated";
}

// a client is announced as it gets its seat, before it is ready - a match starting at once waits for it
TEST_F(NetworkTest, TheServerSaysAClientSatDownBeforeItIsReady)
{
	std::optional<PlayerSlot> seated{};
	const EventSubscription seatedSub{_serverEvents->AddListener([&seated](const ServerClientSeatedEvent& event)
	{
		seated = event.slot;
	})};

	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};

	ASSERT_TRUE(PumpUntil([&seated] { return seated.has_value(); })) << "the server never said the client sat down";
	EXPECT_EQ(*seated, PlayerSlot::P1);
}

// a client answers the field it was sent - the server holds a joined match until it does
TEST_F(NetworkTest, AClientAnswersTheFieldOnceItHasIt)
{
	std::optional<PlayerSlot> synced{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_serverEvents->AddListener([](const WorldSnapshotRequestedEvent& event)
	{
		event.snapshot.phase = GameState::Playing;
	}));
	subs.push_back(_serverEvents->AddListener([&synced](const ServerClientSyncedEvent& event)
	{
		synced = event.slot;
	}));
	subs.push_back(AnnounceReadyOnConnect());

	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};

	ASSERT_TRUE(PumpUntil([&synced] { return synced.has_value(); })) << "the client never said the field reached it";
	EXPECT_EQ(*synced, PlayerSlot::P1);
}

// a seat changing hands sends every client the field, not only the one who sat down
TEST_F(NetworkTest, ASeatTakenInARunningMatchSendsEveryoneTheField)
{
	std::optional<PlayerSlot> firstSeat{};
	std::optional<PlayerSlot> secondSeat{};
	bool isFirstSent{};
	bool isSecondSent{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_serverEvents->AddListener([](const WorldSnapshotRequestedEvent& event)
	{
		event.snapshot.phase = GameState::Playing;
	}));
	subs.push_back(_clientEvents->AddListener([&firstSeat](const PlayerSlotAssignedEvent& e) { firstSeat = e.slot; }));
	subs.push_back(_secondClientEvents->AddListener(
			[&secondSeat](const PlayerSlotAssignedEvent& e) { secondSeat = e.slot; }));
	subs.push_back(_clientEvents->AddListener([&isFirstSent](const WorldSnapshotReceivedEvent&)
	{
		isFirstSent = true;
	}));
	subs.push_back(_secondClientEvents->AddListener(
			[&isSecondSent](const WorldSnapshotReceivedEvent&) { isSecondSent = true; }));

	const auto server{MakeServer()};
	const uint16_t port{server->GetBoundPort()};
	const auto client{MakeClient(port)};
	const auto secondClient{std::make_unique<network::commands::ClientNode>(network::ServerAddress{.port = port},
																			_secondClientEvents)};
	ASSERT_TRUE(PumpUntil([&firstSeat, &secondSeat] { return firstSeat && secondSeat; }));
	ASSERT_FALSE(isFirstSent || isSecondSent) << "the control failed - nobody readied, nobody is owed the field";

	_serverEvents->EmitEvent(SeatHolderChangedEvent{.slot = PlayerSlot::P2, .from = SeatHolder::Bot});

	EXPECT_TRUE(PumpUntil([&isFirstSent, &isSecondSent] { return isFirstSent && isSecondSent; }));
}

//NOTE: the next level is played on a map the launch never named - a client joining then is told the one it is on
TEST_F(NetworkTest, AClientSeatedAfterTheMapChangedIsToldTheNewOne)
{
	std::optional<PlayerSlotAssignedEvent> assigned{};
	const EventSubscription assignedSub{_clientEvents->AddListener(
			[&assigned](const PlayerSlotAssignedEvent& e) { assigned = e; })};

	const auto server{std::make_unique<network::commands::ServerNode>(network::ServerAddress{.port = 0},
																	  _serverEvents, MatchSettings{.map = "level1"})};
	_serverEvents->EmitEvent(MapLoadedEvent{.cols = 52u, .rows = 52u, .stage = 2u, .name = "level2"});
	const auto client{MakeClient(server->GetBoundPort())};

	ASSERT_TRUE(PumpUntil([&assigned] { return assigned.has_value(); })) << "the client was told no seat";
	EXPECT_EQ(assigned->match.map, "level2");
}

// who left goes out to the clients like a phase does - the panel is drawn from it
TEST_F(NetworkTest, TheClientsAreToldWhoLeft)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	std::optional<AbsenceChangedEvent> told{};
	const EventSubscription toldSub{_clientEvents->AddListener([&told](const AbsenceChangedEvent& event)
	{
		told = event;
	})};
	constexpr std::array sent{Absence::None, Absence::Left, Absence::None, Absence::Back};
	_serverEvents->EmitEvent(AbsenceChangedEvent{.seats = sent});

	ASSERT_TRUE(PumpUntil([&told] { return told.has_value(); }));
	EXPECT_EQ(told->seats, sent);
}

// and each answer reaches the server as the one it was
TEST_F(NetworkTest, EveryAnswerAboutWhoLeftReachesTheServer)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));

	std::vector<AbsenceChoice> heard{};
	const EventSubscription heardSub{_serverEvents->AddListener([&heard](const AbsenceChosenEvent& event)
	{
		heard.push_back(event.choice);
	})};
	const std::vector sent{AbsenceChoice::Continue, AbsenceChoice::Bot};
	for (const AbsenceChoice choice: sent)
	{
		_clientEvents->EmitEvent(AbsenceChosenEvent{.choice = choice});
	}

	ASSERT_TRUE(PumpUntil([&heard, &sent] { return heard.size() == sent.size(); }));
	EXPECT_EQ(heard, sent);
}

// a client dialling back gets the seat it held, not the first free one - the one who left comes back to its own tank
TEST_F(NetworkTest, AClientDialingAgainGetsItsOwnSeatOverAnEarlierFreeOne)
{
	bool isGoodbyeHeard{};
	const EventSubscription goodbyeSub{_serverEvents->AddListener([&isGoodbyeHeard](const ServerInDisconnectEvent&)
	{
		isGoodbyeHeard = true;
	})};

	const auto server{MakeServer()};
	auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([&client] { return client->IsConnected(); }));
	boost::asio::io_context ioContext;
	boost::asio::ip::udp::socket socket{ioContext, boost::asio::ip::udp::v4()};
	socket.connect({boost::asio::ip::make_address("127.0.0.1"), server->GetBoundPort()});
	socket.non_blocking(true);
	ASSERT_EQ(Dial(socket, 1u), PlayerSlot::P2);

	client.reset();
	ASSERT_TRUE(PumpUntil([&isGoodbyeHeard] { return isGoodbyeHeard; })) << "the first seat was never let go";

	EXPECT_EQ(Dial(socket, 2u), PlayerSlot::P2) << "the client came back to the first free seat, not its own";
}
