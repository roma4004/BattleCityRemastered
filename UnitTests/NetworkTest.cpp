#include "geometry/ObjRectangle.h"
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
#include "entities/EffectTarget.h"
#include "enums/Absence.h"
#include "enums/Author.h"
#include "enums/Faction.h"
#include "enums/MatchRules.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/DespawnReason.h"
#include "enums/DisconnectReason.h"
#include "enums/GameState.h"
#include "enums/InputChannel.h"
#include "enums/ObstacleType.h"
#include "enums/PlayerSlot.h"
#include "enums/PortForwarding.h"
#include "enums/TankModel.h"
#include "enums/TankType.h"
#include "network/ClientNode.h"
#include "network/DatagramLink.h"
#include "network/Endpoints.h"
#include "network/Serializer.h"
#include "network/commands/CommandBatch.h"
#include "network/commands/SlotAssignment.h"
#include "network/commands/WorldSnapshot.h"
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
#include <tuple>
#include <variant>
#include <vector>
#include "utils/UuidUtils.h"
#include "TestUtils.h"//NOTE: PrintTo for the Point types

using namespace std::chrono_literals;

// a real server and client over loopback: the host emits a local event and the client is asked if it arrived
class NetworkTest : public testing::Test
{
protected:
	Uuid _uuid{UuidUtils::GetRandomUuid()};

	void SetUp() override
	{
		_subs.push_back(_clientEvents->AddListener([this](const ClientConnectedToHostEvent&) { ++_linksUp; }));
		_subs.push_back(_clientEvents->AddListener([this](const PlayerSlotAssignedEvent& event)
		{
			_firstSeat = event.slot;
		}));
		_subs.push_back(_secondClientEvents->AddListener([this](const PlayerSlotAssignedEvent& event)
		{
			_secondSeat = event.slot;
		}));
	}

	//NOTE: one bus per node - on a shared one a listener fires off the local emit, with no networking at all
	std::shared_ptr<EventSystem> _serverEvents{std::make_shared<EventSystem>()};
	std::shared_ptr<EventSystem> _clientEvents{std::make_shared<EventSystem>()};
	//NOTE: a seat is handed out per connection, so telling the two apart takes a second game process
	std::shared_ptr<EventSystem> _secondClientEvents{std::make_shared<EventSystem>()};
	//NOTE: the one that finds both seats taken
	std::shared_ptr<EventSystem> _thirdClientEvents{std::make_shared<EventSystem>()};
	//NOTE: every client made on _clientEvents adds one
	int _linksUp{};
	std::optional<PlayerSlot> _firstSeat{};
	std::optional<PlayerSlot> _secondSeat{};
	std::vector<EventSubscription> _subs{};
	double _deltaTimeOneFrame{1.0 / 60.0};

	//NOTE: stands in for Game::Run, in its order - nothing is received or sent without it
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
		_serverEvents->EmitEvent(SendNetworkFrameEvent{});
		_clientEvents->EmitEvent(SendNetworkFrameEvent{});
		_secondClientEvents->EmitEvent(SendNetworkFrameEvent{});
		_thirdClientEvents->EmitEvent(SendNetworkFrameEvent{});
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
	[[nodiscard]] std::unique_ptr<network::ServerNode> MakeServer(const MatchSettings& match = {}) const
	{
		return std::make_unique<network::ServerNode>(network::ServerAddress{.port = 0}, _serverEvents, match);
	}

	[[nodiscard]] std::unique_ptr<network::ClientNode> MakeClient(const uint16_t port) const
	{
		return MakeClient(port, _clientEvents);
	}

	[[nodiscard]] static std::unique_ptr<network::ClientNode> MakeClient(const uint16_t port,
																		 const std::shared_ptr<EventSystem>& events)
	{
		return std::make_unique<network::ClientNode>(network::ServerAddress{.port = port}, events);
	}

	[[nodiscard]] EventSubscription AnnounceReadyOnConnect() const
	{
		return _clientEvents->AddListener([this](const ClientConnectedToHostEvent&)
		{
			_clientEvents->EmitEvent(ClientOutReadyToPlayEvent{});
		});
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
};

// what the host emits comes out on the client as the same event, keyed the same and whole - a crossed wire swaps them
TEST_F(NetworkTest, EveryReplicatedEventArrivesAsItself)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([this] { return _linksUp == 1; }));

	constexpr FPoint pos{.x = 42.0, .y = 42.0};
	constexpr ObjRectangle grown{.x = 98.0, .y = 96.0, .w = 40.0, .h = 40.0};
	constexpr std::array kTankTypes{TankType::PLAYER1, TankType::PLAYER2, TankType::ENEMY1,
									TankType::ENEMY2, TankType::ENEMY3, TankType::ENEMY4};

	std::optional<PosChangedEvent> moved{};
	std::optional<TankShotEvent> shot{};
	std::optional<HealthChangedEvent> health{};
	std::vector<DespawnedEvent> despawned{};
	std::optional<BonusSpawnedEvent> bonus{};
	std::optional<BonusSpawnCompletedEvent> bonusLanded{};
	std::optional<BonusHelmetAppliedEvent> helmet{};
	std::optional<BonusShipAppliedEvent> ship{};
	std::optional<BonusTankAppliedEvent> extraLife{};
	std::optional<BonusEffectsChangedEvent> effects{};
	std::optional<TierChangedEvent> tier{};
	std::optional<TankGrewEvent> grew{};
	std::vector<TankRespawnedEvent> respawned{};
	std::optional<StatisticsBulletHitEvent> bulletHit{};
	std::optional<StatisticsTankHitEvent> tankHit{};
	std::optional<TankDiedEvent> tankDied{};
	std::optional<BrickWallDiedEvent> brickDied{};
	std::optional<SteelWallDiedEvent> steelDied{};
	std::optional<StatisticsBonusPickupEvent> bonusPickup{};
	std::optional<StatisticsBonusDestroyedEvent> bonusDestroyed{};
	bool bonusExpired{};

	std::vector<EventSubscription> subs{};
	subs.push_back(_clientEvents->AddListener(Key(_uuid), [&moved](const PosChangedEvent& e) { moved = e; }));
	subs.push_back(_clientEvents->AddListener(Key(Author::Player1), [&shot](const TankShotEvent& e) { shot = e; }));
	subs.push_back(_clientEvents->AddListener(Key(_uuid), [&health](const HealthChangedEvent& e) { health = e; }));
	subs.push_back(_clientEvents->AddListener(Key(_uuid),
											  [&despawned](const DespawnedEvent& e) { despawned.push_back(e); }));
	subs.push_back(_clientEvents->AddListener([&bonus](const BonusSpawnedEvent& e) { bonus = e; }));
	subs.push_back(_clientEvents->AddListener([&bonusLanded](const BonusSpawnCompletedEvent& e) { bonusLanded = e; }));
	subs.push_back(_clientEvents->AddListener(Key(Author::Player1),
											  [&helmet](const BonusHelmetAppliedEvent& e) { helmet = e; }));
	subs.push_back(_clientEvents->AddListener(Key(Author::Player1),
											  [&ship](const BonusShipAppliedEvent& e) { ship = e; }));
	subs.push_back(_clientEvents->AddListener([&extraLife](const BonusTankAppliedEvent& e) { extraLife = e; }));
	subs.push_back(_clientEvents->AddListener([&effects](const BonusEffectsChangedEvent& e) { effects = e; }));
	subs.push_back(_clientEvents->AddListener(Key(_uuid), [&tier](const TierChangedEvent& e) { tier = e; }));
	subs.push_back(_clientEvents->AddListener(Key(_uuid), [&grew](const TankGrewEvent& e) { grew = e; }));
	subs.push_back(_clientEvents->AddListener([&respawned](const TankRespawnedEvent& e) { respawned.push_back(e); }));
	subs.push_back(_clientEvents->AddListener([&bulletHit](const StatisticsBulletHitEvent& e) { bulletHit = e; }));
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

	_serverEvents->EmitEvent(PosChangedEvent{.pos = pos, .dir = Direction::UP, .uuid = _uuid});
	_serverEvents->EmitEvent(TankShotEvent{.who = Author::Player1, .dir = Direction::LEFT, .bulletUuid = _uuid});
	_serverEvents->EmitEvent(HealthChangedEvent{.health = 42u, .uuid = _uuid});
	//NOTE: two despawns - a destroyed bullet and a picked-up bonus travel the same command
	_serverEvents->EmitEvent(DespawnedEvent{.uuid = _uuid, .reason = DespawnReason::Destroyed});
	_serverEvents->EmitEvent(DespawnedEvent{.uuid = _uuid, .reason = DespawnReason::PickedUp});
	_serverEvents->EmitEvent(BonusSpawnedEvent{.pos = pos, .type = BonusType::Timer, .uuid = _uuid, .isSuper = true});
	_serverEvents->EmitEvent(BonusSpawnCompletedEvent{.uuid = _uuid});
	//NOTE: three effects share BonusStatus, told apart by bonusType - a swap lands the wrong effect on the wrong seat
	_serverEvents->EmitEvent(BonusHelmetAppliedEvent{.author = Author::Player1, .isActive = true});
	_serverEvents->EmitEvent(BonusShipAppliedEvent{.author = Author::Player1});
	_serverEvents->EmitEvent(BonusTankAppliedEvent{.author = Author::Player2});
	_serverEvents->EmitEvent(BonusEffectsChangedEvent{
			.effects = {BonusEffectTimeLeft{.type = BonusType::Helmet,
											.target = Author::Enemy3,
											.duration = 20s,
											.left = 7s},
						BonusEffectTimeLeft{.type = BonusType::Timer,
											.target = Faction::Solo,
											.spared = Author::Player2,
											.duration = 15s,
											.left = 15s}}});
	//NOTE: the tier travels as a result on its own command - the client sets it rather than replaying the upgrade
	_serverEvents->EmitEvent(TierChangedEvent{.tier = 4u, .uuid = _uuid});
	_serverEvents->EmitEvent(TankGrewEvent{.rect = grown, .uuid = _uuid});
	for (const TankType type: kTankTypes)
	{
		_serverEvents->EmitEvent(
				TankRespawnedEvent{.type = type, .model = TankModel::Basic, .uuid = _uuid, .pos = pos});
	}
	//NOTE: eleven local types share one StatisticsChange - each case has its own author, or a crossed wire would hide
	_serverEvents->EmitEvent(StatisticsBulletHitEvent{.author = Author::Player3});
	_serverEvents->EmitEvent(StatisticsTankHitEvent{.who = Author::Player1, .author = Author::Enemy1});
	_serverEvents->EmitEvent(TankDiedEvent{.who = Author::Player2, .uuid = _uuid, .author = Author::Enemy2});
	_serverEvents->EmitEvent(BrickWallDiedEvent{.author = Author::Enemy3});
	_serverEvents->EmitEvent(SteelWallDiedEvent{.author = Author::Enemy4});
	_serverEvents->EmitEvent(StatisticsBonusPickupEvent{.author = Author::Player1});
	_serverEvents->EmitEvent(StatisticsBonusDestroyedEvent{.author = Author::Player2});
	_serverEvents->EmitEvent(StatisticsBonusExpiredEvent{});

	ASSERT_TRUE(PumpUntil([&]
	{
		return moved && shot && health && despawned.size() == 2u && bonus && bonusLanded && helmet && ship && extraLife
			   && effects && tier && grew && respawned.size() == kTankTypes.size() && bulletHit && tankHit
			   && tankDied && brickDied && steelDied && bonusPickup && bonusDestroyed && bonusExpired;
	}));

	EXPECT_EQ(moved->pos, pos);
	EXPECT_EQ(moved->dir, Direction::UP);
	EXPECT_EQ(shot->dir, Direction::LEFT);
	EXPECT_EQ(shot->bulletUuid, _uuid);
	EXPECT_EQ(health->health, 42u);
	EXPECT_EQ(despawned[0].reason, DespawnReason::Destroyed);
	EXPECT_EQ(despawned[1].reason, DespawnReason::PickedUp);
	EXPECT_EQ(bonus->pos, pos);
	EXPECT_EQ(bonus->type, BonusType::Timer);
	EXPECT_EQ(bonus->uuid, _uuid);
	EXPECT_TRUE(bonus->isSuper);
	EXPECT_EQ(bonusLanded->uuid, _uuid);
	EXPECT_TRUE(helmet->isActive);
	EXPECT_EQ(ship->author, Author::Player1);
	EXPECT_EQ(extraLife->author, Author::Player2);
	ASSERT_EQ(effects->effects.size(), 2u);
	EXPECT_EQ(effects->effects[0].type, BonusType::Helmet);
	EXPECT_EQ(effects->effects[0].target, EffectTarget{Author::Enemy3});
	EXPECT_EQ(effects->effects[0].duration, 20s);
	EXPECT_EQ(effects->effects[0].left, 7s);
	EXPECT_EQ(effects->effects[1].type, BonusType::Timer);
	EXPECT_EQ(effects->effects[1].target, EffectTarget{Faction::Solo});
	EXPECT_EQ(effects->effects[1].spared, Author::Player2);
	EXPECT_EQ(tier->tier, 4u);
	EXPECT_DOUBLE_EQ(grew->rect.x, grown.x);
	EXPECT_DOUBLE_EQ(grew->rect.y, grown.y);
	EXPECT_DOUBLE_EQ(grew->rect.w, grown.w);
	for (std::size_t i{}; i < kTankTypes.size(); ++i)
	{
		EXPECT_EQ(respawned[i].type, kTankTypes[i]);
		EXPECT_EQ(respawned[i].uuid, _uuid);
		EXPECT_EQ(respawned[i].pos, pos);
	}
	EXPECT_EQ(bulletHit->author, Author::Player3);
	EXPECT_EQ(tankHit->who, Author::Player1);
	EXPECT_EQ(tankHit->author, Author::Enemy1);
	EXPECT_EQ(tankDied->who, Author::Player2);
	EXPECT_EQ(tankDied->uuid, _uuid);
	EXPECT_EQ(tankDied->author, Author::Enemy2);
	EXPECT_EQ(brickDied->author, Author::Enemy3);
	EXPECT_EQ(steelDied->author, Author::Enemy4);
	EXPECT_EQ(bonusPickup->author, Author::Player1);
	EXPECT_EQ(bonusDestroyed->author, Author::Player2);
}

// one of the four things a client may ask for: the pause travels the other way and stops the host
TEST_F(NetworkTest, PauseRequestFromClientPausesTheServer)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([this] { return _linksUp == 1; }));

	bool isServerPaused{};
	const EventSubscription pauseSub{_serverEvents->AddListener(
			[&isServerPaused](const SetPauseEvent& event) { isServerPaused = event.isPaused; })};

	_clientEvents->EmitEvent(PauseRequestedEvent{.isPaused = true});

	EXPECT_TRUE(PumpUntil([&isServerPaused] { return isServerPaused; }));

	//NOTE: the state the client asked for, not a toggle - a second client must not undo the first one's pause
	_clientEvents->EmitEvent(PauseRequestedEvent{.isPaused = false});

	EXPECT_TRUE(PumpUntil([&isServerPaused] { return !isServerPaused; }));
}

//NOTE: a whole map in one batch - one frame, and every command has to come out of it in order
TEST_F(NetworkTest, MassiveObstacleSpawnEventReplication)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([this] { return _linksUp == 1; }));

	constexpr auto obstacleType{ObstacleType::Brick};
	constexpr unsigned short itemsInMassiveTest{10000u};

	std::vector<FPoint> sent{};
	sent.reserve(itemsInMassiveTest);
	for (std::size_t i{}; i < itemsInMassiveTest; ++i)
	{
		const auto value{static_cast<double>(i)};
		sent.emplace_back(FPoint{.x = value, .y = value + 1.0});
	}

	std::vector<ObstacleSpawnedEvent> received{};
	received.reserve(itemsInMassiveTest);
	const EventSubscription massiveObstacleSub{_clientEvents->AddListener(
			[&received](const ObstacleSpawnedEvent& event) { received.push_back(event); })};

	for (const auto& pos: sent)
	{
		_serverEvents->EmitEvent(ObstacleSpawnedEvent{.pos = pos, .type = obstacleType, .uuid = _uuid});
	}

	ASSERT_TRUE(PumpUntil([&received] { return received.size() == itemsInMassiveTest; }))
			<< "only " << received.size() << " of " << itemsInMassiveTest << " obstacles arrived";

	for (std::size_t i{}; i < itemsInMassiveTest; ++i)
	{
		ASSERT_EQ(sent[i], received[i].pos) << "obstacle " << i << " arrived out of order or damaged";
		ASSERT_EQ(obstacleType, received[i].type);
		ASSERT_EQ(_uuid, received[i].uuid);
	}
}

// a client that quits says why as well, so the seat is freed rather than timed out
TEST_F(NetworkTest, ClientQuitTellsHostWhy)
{
	const auto server{MakeServer()};
	auto client{MakeClient(server->GetBoundPort())};

	ASSERT_TRUE(PumpUntil([this] { return _linksUp == 1; }));

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

// the seat given on connect is all that ties a key to a tank - the server reads whose press it is off the session
TEST_F(NetworkTest, TheServerHandsOutTheSeatsInOrder)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([this] { return _firstSeat.has_value(); })) << "the first client was told no seat";
	//NOTE: dialled only once the first seat is out, so the accept order is the test's, not the OS's
	const auto secondClient{MakeClient(server->GetBoundPort(), _secondClientEvents)};
	ASSERT_TRUE(PumpUntil([this] { return _secondSeat.has_value(); })) << "the second client was told no seat";

	EXPECT_EQ(PlayerSlot::P1, *_firstSeat);
	EXPECT_EQ(PlayerSlot::P2, *_secondSeat);
}

// a seat comes back only once its session is swept - one that said goodbye is swept on its own IsFinished
TEST_F(NetworkTest, ASeatComesBackWhenItsClientSaysGoodbye)
{
	const auto server{MakeServer()};
	auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([this] { return _firstSeat.has_value(); })) << "the first client was told no seat";
	ASSERT_EQ(PlayerSlot::P1, *_firstSeat);

	bool goodbye{};
	const EventSubscription goodbyeSub{_serverEvents->AddListener(
			[&goodbye](const ServerInDisconnectEvent&) { goodbye = true; })};

	client.reset();
	ASSERT_TRUE(PumpUntil([&goodbye] { return goodbye; })) << "host never got the client's goodbye";

	const auto next{MakeClient(server->GetBoundPort(), _secondClientEvents)};
	ASSERT_TRUE(PumpUntil([this] { return _secondSeat.has_value(); })) << "the freed seat was never handed out";
	EXPECT_EQ(PlayerSlot::P1, *_secondSeat) << "the seat of a client that quit is still held by its session";
}

// two clients on one machine: each tags its presses with its own seat, so neither drives the other's tank
TEST_F(NetworkTest, EachClientPutsOnlyItsOwnSeatOnTheWire)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([this] { return _firstSeat.has_value(); }));
	const auto secondClient{MakeClient(server->GetBoundPort(), _secondClientEvents)};
	ASSERT_TRUE(PumpUntil([this] { return _secondSeat.has_value(); }));

	bool firstSeatMoved{};
	bool secondSeatMoved{};
	bool strayArrived{};
	std::vector<EventSubscription> subs{};
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

	//NOTE: the stray goes first - the wire keeps order, so once the press behind it lands, the stray never left
	_clientEvents->EmitEvent(Key(InputChannel::LocalP2), MoveDownEvent{.isPressed = true});
	_clientEvents->EmitEvent(Key(InputChannel::LocalP1), MoveUpEvent{.isPressed = true});
	_secondClientEvents->EmitEvent(Key(InputChannel::LocalP2), MoveUpEvent{.isPressed = true});

	ASSERT_TRUE(PumpUntil([&firstSeatMoved, &secondSeatMoved] { return firstSeatMoved && secondSeatMoved; }));
	EXPECT_FALSE(strayArrived) << "a client sent the keyboard half of a seat it was never given";
}

//NOTE: an unanswered hello reads as no host - a third player would spend its retries on a server merely busy
TEST_F(NetworkTest, AThirdClientIsToldTheSeatsAreTaken)
{
	std::optional<DisconnectReason> refusal{};
	const EventSubscription refusalSub{_thirdClientEvents->AddListener(
			[&refusal](const ClientInDisconnectEvent& e) { refusal = e.reason; })};

	const auto server{MakeServer()};
	const uint16_t port{server->GetBoundPort()};
	const auto client{MakeClient(port)};
	const auto secondClient{MakeClient(port, _secondClientEvents)};
	ASSERT_TRUE(PumpUntil([this] { return _firstSeat && _secondSeat; }));

	const auto thirdClient{MakeClient(port, _thirdClientEvents)};

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
	const auto server{MakeServer(match)};
	const uint16_t port{server->GetBoundPort()};
	const auto client{MakeClient(port)};
	const auto secondClient{MakeClient(port, _secondClientEvents)};
	const auto thirdClient{MakeClient(port, _thirdClientEvents)};

	ASSERT_TRUE(PumpUntil([&third] { return third.has_value(); })) << "the third client was told no seat";
	EXPECT_EQ(third->slot, PlayerSlot::P3);
	EXPECT_EQ(third->match, match);
}

// a ready in a running match is owed the field, sent in place of that frame - beside it, spawns would land twice
TEST_F(NetworkTest, AReadyInARunningMatchIsAnsweredWithTheFieldInsteadOfTheFrame)
{
	const Uuid standing{UuidUtils::GetRandomUuid()};
	const Uuid inTheFrame{UuidUtils::GetRandomUuid()};
	std::optional<std::vector<network::commands::ObstacleSnapshot>> field{};
	std::vector<Uuid> spawned{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_serverEvents->AddListener([standing](const WorldSnapshotRequestedEvent& event)
	{
		event.snapshot.phase = GameState::Playing;
		event.snapshot.obstacles.push_back(
				network::commands::ObstacleSnapshot{.pos = {}, .type = ObstacleType::Steel, .uuid = standing});
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

// a lobby ready owes the field once the match starts - otherwise the first one in plays a field nobody sends
TEST_F(NetworkTest, AReadyOwedFromTheLobbyIsPaidOnceTheMatchStarts)
{
	const Uuid standing{UuidUtils::GetRandomUuid()};
	const Uuid inTheFrame{UuidUtils::GetRandomUuid()};
	auto phase{GameState::Lobby};
	std::optional<std::vector<network::commands::ObstacleSnapshot>> field{};
	std::optional<Uuid> spawned{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_serverEvents->AddListener([&phase, standing](const WorldSnapshotRequestedEvent& event)
	{
		event.snapshot.phase = phase;
		event.snapshot.obstacles.push_back(
				network::commands::ObstacleSnapshot{.pos = {}, .type = ObstacleType::Steel, .uuid = standing});
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

// a client redialing from the same socket before the host saw the timeout gets its seat back, not "match full"
TEST_F(NetworkTest, AClientDialingAgainFromTheSameAddressTakesItsSeatBack)
{
	std::optional<PlayerSlot> lost{};
	const EventSubscription lostSub{_serverEvents->AddListener(
			[&lost](const ServerClientLostEvent& event) { lost = event.slot; })};

	const auto server{MakeServer()};
	boost::asio::io_context ioContext;
	boost::asio::ip::udp::socket socket{ioContext, boost::asio::ip::udp::v4()};
	socket.connect({boost::asio::ip::make_address("127.0.0.1"), server->GetBoundPort()});
	socket.non_blocking(true);

	ASSERT_EQ(Dial(socket, 1u), PlayerSlot::P1);
	const auto secondClient{MakeClient(server->GetBoundPort(), _secondClientEvents)};
	ASSERT_TRUE(PumpUntil([this] { return _secondSeat.has_value(); }));

	EXPECT_EQ(Dial(socket, 2u), PlayerSlot::P1) << "the new connection was refused a seat its own old one held";
	ASSERT_TRUE(PumpUntil([&lost] { return lost.has_value(); })) << "the replaced connection was never let go";
	EXPECT_EQ(*lost, PlayerSlot::P1);
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
	ASSERT_TRUE(PumpUntil([this] { return _linksUp == 1; }));

	_serverEvents->EmitEvent(ServerKickRequestedEvent{.slot = PlayerSlot::P1});

	ASSERT_TRUE(PumpUntil([&reason, &isAbandoned, &isSeatFreed] { return reason && isAbandoned && isSeatFreed; }))
			<< "told why: " << reason.has_value() << ", gave up: " << isAbandoned << ", seat freed: " << isSeatFreed;
	EXPECT_EQ(*reason, DisconnectReason::Kicked);
}

// a hello the server turns away is no connection - the client must not report a seat it never got
TEST_F(NetworkTest, ATurnedAwayHelloIsNoConnection)
{
	const auto server{MakeServer()};
	_serverEvents->EmitEvent(ServerAcceptingChangedEvent{.isAccepting = false});

	std::optional<DisconnectReason> refusal{};
	const EventSubscription refusalSub{_clientEvents->AddListener(
			[&refusal](const ClientInDisconnectEvent& event) { refusal = event.reason; })};

	const auto client{MakeClient(server->GetBoundPort())};

	ASSERT_TRUE(PumpUntil([&refusal] { return refusal.has_value(); })) << "the closed server never turned it away";
	EXPECT_EQ(DisconnectReason::ServerFull, *refusal);
	EXPECT_EQ(0, _linksUp) << "a turned-away client told its own side the link was up";
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
	bool isFirstSent{};
	bool isSecondSent{};
	std::vector<EventSubscription> subs{};
	subs.push_back(_serverEvents->AddListener([](const WorldSnapshotRequestedEvent& event)
	{
		event.snapshot.phase = GameState::Playing;
	}));
	subs.push_back(_clientEvents->AddListener([&isFirstSent](const WorldSnapshotReceivedEvent&)
	{
		isFirstSent = true;
	}));
	subs.push_back(_secondClientEvents->AddListener(
			[&isSecondSent](const WorldSnapshotReceivedEvent&) { isSecondSent = true; }));

	const auto server{MakeServer()};
	const uint16_t port{server->GetBoundPort()};
	const auto client{MakeClient(port)};
	const auto secondClient{MakeClient(port, _secondClientEvents)};
	ASSERT_TRUE(PumpUntil([this] { return _firstSeat && _secondSeat; }));
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

	const auto server{MakeServer(MatchSettings{.map = "level1"})};
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
	ASSERT_TRUE(PumpUntil([this] { return _linksUp == 1; }));

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

// what the router said about the server's port goes out to the clients - the lobby gives a friend the way in from it
TEST_F(NetworkTest, TheClientsAreToldWhereTheInternetReachesTheServer)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([this] { return _linksUp == 1; }));

	std::optional<PortForwardingChangedEvent> told{};
	const EventSubscription toldSub{_clientEvents->AddListener([&told](const PortForwardingChangedEvent& event)
	{
		told = event;
	})};
	_serverEvents->EmitEvent(
			PortForwardingChangedEvent{.state = PortForwarding::Open, .host = "203.0.113.7", .port = 5000u});

	ASSERT_TRUE(PumpUntil([&told] { return told.has_value(); }));
	EXPECT_EQ(told->state, PortForwarding::Open);
	EXPECT_EQ(told->host, "203.0.113.7");
	EXPECT_EQ(told->port, 5000u);
}

// a client that sits down after the router answered is told it all the same
TEST_F(NetworkTest, AClientSeatedAfterTheRouterAnsweredIsToldTheWayIn)
{
	std::optional<PortForwardingChangedEvent> told{};
	const EventSubscription toldSub{_clientEvents->AddListener([&told](const PortForwardingChangedEvent& event)
	{
		told = event;
	})};

	const auto server{MakeServer()};
	_serverEvents->EmitEvent(PortForwardingChangedEvent{.state = PortForwarding::Refused, .port = 5000u});
	Pump();
	const auto client{MakeClient(server->GetBoundPort())};

	ASSERT_TRUE(PumpUntil([&told] { return told.has_value(); }));
	EXPECT_EQ(told->state, PortForwarding::Refused);
	EXPECT_EQ(told->port, 5000u);
}

// and each answer reaches the server as the one it was
TEST_F(NetworkTest, EveryAnswerAboutWhoLeftReachesTheServer)
{
	const auto server{MakeServer()};
	const auto client{MakeClient(server->GetBoundPort())};
	ASSERT_TRUE(PumpUntil([this] { return _linksUp == 1; }));

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
	ASSERT_TRUE(PumpUntil([this] { return _linksUp == 1; }));
	boost::asio::io_context ioContext;
	boost::asio::ip::udp::socket socket{ioContext, boost::asio::ip::udp::v4()};
	socket.connect({boost::asio::ip::make_address("127.0.0.1"), server->GetBoundPort()});
	socket.non_blocking(true);
	ASSERT_EQ(Dial(socket, 1u), PlayerSlot::P2);

	client.reset();
	ASSERT_TRUE(PumpUntil([&isGoodbyeHeard] { return isGoodbyeHeard; })) << "the first seat was never let go";

	EXPECT_EQ(Dial(socket, 2u), PlayerSlot::P2) << "the client came back to the first free seat, not its own";
}
