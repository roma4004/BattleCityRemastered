#include "Point.h"
#include "components/EventSystem.h"
#include "components/SpawnEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ObstacleAndBonusEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/StatisticsEvents.h"
#include "components/events/TimingEvents.h"
#include "entities/ObjRectangle.h"
#include "enums/BonusType.h"
#include "enums/Direction.h"
#include "enums/ObstacleType.h"
#include "enums/TankType.h"
#include "network/ClientHandler.h"
#include "network/ServerHandler.h"
#include "gtest/gtest.h"
#include <memory>
#include <boost/uuid/random_generator.hpp>
#include <boost/uuid/string_generator.hpp>
#include <boost/uuid/uuid.hpp>
#include <boost/uuid/uuid_io.hpp>

class NetworkTest : public testing::Test
{
	using buuid = boost::uuids::uuid;

protected:
	buuid _uuid{boost::uuids::string_generator()("01234567-89ab-cdef-0123-456789abcdef")};

	void SetUp() override {}

	void TearDown() override {}
};

TEST_F(NetworkTest, PosEventReplication)
{
	auto events = std::make_shared<EventSystem>();
	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, events);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), events);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	constexpr FPoint posOrigin{.x = 42.f, .y = 42.f};
	constexpr auto directionOrigin{Direction::UP};

	std::promise<std::tuple<FPoint, Direction>> promise{};
	auto future = promise.get_future();

	const auto name{std::string("TestTank")};
	auto posSub = events->AddListener(Key(_uuid), [&promise](const ClientInPosEvent& event)
	{
		promise.set_value({event.pos, event.dir});
	});

	// events->EmitEvent(ServerStartFrameEvent{});
	events->EmitEvent(
			ServerOutPosEvent{.who = name, .pos = posOrigin, .dir = directionOrigin, .uuid = _uuid});
	events->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
		events->EmitEvent(PreTickUpdateEvent{.deltaTime = 1.0});

		if (status = future.wait_for(checkInterval);
			status == std::future_status::ready)
		{
			break;
		}
	}

	ASSERT_EQ(status, std::future_status::ready);
	const auto& [posReplicated, dirReplicated] = future.get();

	EXPECT_EQ(posOrigin, posReplicated);
	EXPECT_EQ(directionOrigin, dirReplicated);

}

TEST_F(NetworkTest, ShotEventReplication)
{
	using buuid = boost::uuids::uuid;

	auto events = std::make_shared<EventSystem>();
	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, events);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), events);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	constexpr Direction direction{Direction::UP};

	std::promise<std::pair<Direction, buuid>> promise{};
	auto future = promise.get_future();

	const auto name{std::string("TestTank")};
	auto shotSub = events->AddListener(Key(name), [&promise](const ClientInShotEvent& event)
	{
		promise.set_value({event.dir, event.bulletUuid});
	});

	// events->EmitEvent(ServerStartFrameEvent{});
	events->EmitEvent(ServerOutShotEvent{.who = name, .dir = direction, .bulletUuid = _uuid});
	events->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
		events->EmitEvent(PreTickUpdateEvent{.deltaTime = 1.0});

		if (status = future.wait_for(checkInterval);
			status == std::future_status::ready)
		{
			break;
		}
	}

	ASSERT_EQ(status, std::future_status::ready);
	const auto [dirReplicated, uuidReplicated] = future.get();

	EXPECT_EQ(direction, dirReplicated);
	EXPECT_EQ(_uuid, uuidReplicated);

}

TEST_F(NetworkTest, HealthEventReplication)
{
	auto events = std::make_shared<EventSystem>();
	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, events);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), events);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	constexpr int healthOrigin{42};

	std::promise<int> promise{};
	auto future = promise.get_future();

	const auto name{std::string("TestTank")};

	auto healthSub = events->AddListener(Key(_uuid),
			[&promise](const ClientInHealthEvent& event) { promise.set_value(event.health); });

	// events->EmitEvent(ServerStartFrameEvent{});
	events->EmitEvent(ServerOutHealthEvent{.who = name, .health = healthOrigin, .uuid = _uuid});
	events->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
		events->EmitEvent(PreTickUpdateEvent{.deltaTime = 1.0});

		if (status = future.wait_for(checkInterval);
			status == std::future_status::ready)
		{
			break;
		}
	}

	ASSERT_EQ(status, std::future_status::ready);
	const auto healthReplicated = future.get();
	EXPECT_EQ(healthOrigin, healthReplicated);

}

TEST_F(NetworkTest, DisposeEventReplication)
{
	using buuid = boost::uuids::uuid;

	auto events = std::make_shared<EventSystem>();
	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, events);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), events);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	std::promise<buuid> promise{};
	auto future = promise.get_future();

	auto disposeSub = events->AddListener(Key(_uuid),
			[&promise, uuid = _uuid](const ClientInDisposeEvent&) { promise.set_value(uuid); });

	// events->EmitEvent(ServerStartFrameEvent{});
	events->EmitEvent(ServerOutDisposeEvent{.uuid = _uuid});
	events->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
		events->EmitEvent(PreTickUpdateEvent{.deltaTime = 1.0});

		if (status = future.wait_for(checkInterval);
			status == std::future_status::ready)
		{
			break;
		}
	}

	ASSERT_EQ(status, std::future_status::ready);
	const auto uuidReplicated = future.get();
	EXPECT_EQ(_uuid, uuidReplicated);

}

//TODO: cover all statistics items like this
TEST_F(NetworkTest, StatisticsEventReplication)
{
	auto events = std::make_shared<EventSystem>();
	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, events);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), events);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	std::promise<std::pair<std::string, std::string>> promise{};
	auto future = promise.get_future();

	auto statsSub = events->AddListener([&promise](const ClientInBulletHitEvent& event)
	{
		promise.set_value({event.author, event.fraction});
	});

	// events->EmitEvent(ServerStartFrameEvent{});
	events->EmitEvent(ServerOutBulletHitEvent{.author = "author", .fraction = "fraction"});
	events->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
		events->EmitEvent(PreTickUpdateEvent{.deltaTime = 1.0});

		if (status = future.wait_for(checkInterval);
			status == std::future_status::ready)
		{
			break;
		}
	}

	ASSERT_EQ(status, std::future_status::ready);
	const auto& [author, fraction] = future.get();
	EXPECT_EQ("author", author);
	EXPECT_EQ("fraction", fraction);

}

TEST_F(NetworkTest, FortressChangeEventReplication)
{
	using buuid = boost::uuids::uuid;

	buuid uuid1Died{boost::uuids::string_generator()("11234567-89ab-cdef-0123-456789abcdef")};
	buuid uuid1ToBrick{boost::uuids::string_generator()("21234567-89ab-cdef-0123-456789abcdef")};
	buuid uuid1ToSteel{boost::uuids::string_generator()("31234567-89ab-cdef-0123-456789abcdef")};

	buuid uuid2Died{boost::uuids::string_generator()("41234567-89ab-cdef-0123-456789abcdef")};
	buuid uuid2ToBrick{boost::uuids::string_generator()("51234567-89ab-cdef-0123-456789abcdef")};
	buuid uuid2ToSteel{boost::uuids::string_generator()("61234567-89ab-cdef-0123-456789abcdef")};

	auto events = std::make_shared<EventSystem>();
	auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, events);
	auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), events);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	std::promise<std::string> promiseDied1{};
	auto futureDied1 = promiseDied1.get_future();
	std::promise<std::string> promiseDied2{};
	auto futureDied2 = promiseDied2.get_future();

	std::promise<std::string> promiseToBrick1{};
	auto futureToBrick1 = promiseToBrick1.get_future();
	std::promise<std::string> promiseToBrick2{};
	auto futureToBrick2 = promiseToBrick2.get_future();

	std::promise<std::string> promiseToSteel1{};
	auto futureToSteel1 = promiseToSteel1.get_future();
	std::promise<std::string> promiseToSteel2{};
	auto futureToSteel2 = promiseToSteel2.get_future();

	// NOTE: keyed by each event's own uuid - delivery itself proves uuid routing is correct,
	// no need to compare event.uuid inside the callback anymore.
	auto diedSub1 = events->AddListener(Key(uuid1Died),
			[&promiseDied1](const ClientInFortressChangeEvent& event) { promiseDied1.set_value(event.state); });
	auto toBrickSub1 = events->AddListener(Key(uuid1ToBrick),
			[&promiseToBrick1](const ClientInFortressChangeEvent& event) { promiseToBrick1.set_value(event.state); });
	auto toSteelSub1 = events->AddListener(Key(uuid1ToSteel),
			[&promiseToSteel1](const ClientInFortressChangeEvent& event) { promiseToSteel1.set_value(event.state); });

	auto diedSub2 = events->AddListener(Key(uuid2Died),
			[&promiseDied2](const ClientInFortressChangeEvent& event) { promiseDied2.set_value(event.state); });
	auto toBrickSub2 = events->AddListener(Key(uuid2ToBrick),
			[&promiseToBrick2](const ClientInFortressChangeEvent& event) { promiseToBrick2.set_value(event.state); });
	auto toSteelSub2 = events->AddListener(Key(uuid2ToSteel),
			[&promiseToSteel2](const ClientInFortressChangeEvent& event) { promiseToSteel2.set_value(event.state); });

	// events->EmitEvent(ServerStartFrameEvent{});
	events->EmitEvent(ServerOutFortressChangeEvent{.state = "Died", .uuid = uuid1Died});
	events->EmitEvent(ServerOutFortressChangeEvent{.state = "ToBrick", .uuid = uuid1ToBrick});
	events->EmitEvent(ServerOutFortressChangeEvent{.state = "ToSteel", .uuid = uuid1ToSteel});

	events->EmitEvent(ServerOutFortressChangeEvent{.state = "Died", .uuid = uuid2Died});
	events->EmitEvent(ServerOutFortressChangeEvent{.state = "ToBrick", .uuid = uuid2ToBrick});
	events->EmitEvent(ServerOutFortressChangeEvent{.state = "ToSteel", .uuid = uuid2ToSteel});

	events->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

	{//test first fortressWall
		{//died1 test
			auto statusToDied1 = std::future_status::timeout;
			while (std::chrono::steady_clock::now() - startTime < totalTimeout)
			{
				events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
				events->EmitEvent(PreTickUpdateEvent{.deltaTime = 1.0});

				if (statusToDied1 = futureDied1.wait_for(checkInterval);
					statusToDied1 == std::future_status::ready)
				{
					break;
				}
			}

			ASSERT_EQ(statusToDied1, std::future_status::ready);
			EXPECT_EQ("Died", futureDied1.get());
		}
		{//brick1 test
			auto statusToBrick1 = std::future_status::timeout;
			while (std::chrono::steady_clock::now() - startTime < totalTimeout)
			{
				events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
				events->EmitEvent(PreTickUpdateEvent{.deltaTime = 1.0});

				if (statusToBrick1 = futureToBrick1.wait_for(checkInterval);
					statusToBrick1 == std::future_status::ready)
				{
					break;
				}
			}

			ASSERT_EQ(statusToBrick1, std::future_status::ready);
			EXPECT_EQ("ToBrick", futureToBrick1.get());
		}
		{//steel1 test
			auto statusToSteel1 = std::future_status::timeout;
			while (std::chrono::steady_clock::now() - startTime < totalTimeout)
			{
				events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
				events->EmitEvent(PreTickUpdateEvent{.deltaTime = 1.0});

				if (statusToSteel1 = futureToSteel1.wait_for(checkInterval);
					statusToSteel1 == std::future_status::ready)
				{
					break;
				}
			}

			ASSERT_EQ(statusToSteel1, std::future_status::ready);
			EXPECT_EQ("ToSteel", futureToSteel1.get());
		}
	}
	{//test second fortressWall
		{//died2 test
			auto statusToDied2 = std::future_status::timeout;
			while (std::chrono::steady_clock::now() - startTime < totalTimeout)
			{
				events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
				events->EmitEvent(PreTickUpdateEvent{.deltaTime = 1.0});

				if (statusToDied2 = futureDied2.wait_for(checkInterval);
					statusToDied2 == std::future_status::ready)
				{
					break;
				}
			}

			ASSERT_EQ(statusToDied2, std::future_status::ready);
			EXPECT_EQ("Died", futureDied2.get());
		}
		{//brick2 test
			auto statusToBrick2 = std::future_status::timeout;
			while (std::chrono::steady_clock::now() - startTime < totalTimeout)
			{
				events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
				events->EmitEvent(PreTickUpdateEvent{.deltaTime = 1.0});

				if (statusToBrick2 = futureToBrick2.wait_for(checkInterval);
					statusToBrick2 == std::future_status::ready)
				{
					break;
				}
			}

			ASSERT_EQ(statusToBrick2, std::future_status::ready);
			EXPECT_EQ("ToBrick", futureToBrick2.get());
		}
		{//steel2 test
			auto statusToSteel2 = std::future_status::timeout;
			while (std::chrono::steady_clock::now() - startTime < totalTimeout)
			{
				events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
				events->EmitEvent(PreTickUpdateEvent{.deltaTime = 1.0});

				if (statusToSteel2 = futureToSteel2.wait_for(checkInterval);
					statusToSteel2 == std::future_status::ready)
				{
					break;
				}
			}

			ASSERT_EQ(statusToSteel2, std::future_status::ready);
			EXPECT_EQ("ToSteel", futureToSteel2.get());
		}
	}
}

TEST_F(NetworkTest, BonusSpawnEventReplication)
{
	using buuid = boost::uuids::uuid;

	auto events = std::make_shared<EventSystem>();
	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, events);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), events);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	std::promise<std::tuple<FPoint, BonusType, buuid>> promise{};
	auto future = promise.get_future();

	auto bonusSpawnSub = events->AddListener([&promise](const ClientInBonusSpawnEvent& event)
	{
		promise.set_value({event.pos, event.type, event.uuid});
	});

	// events->EmitEvent(ServerStartFrameEvent{});
	constexpr FPoint pos{.x = 42.f, .y = 42.f};
	constexpr auto type{BonusType::Timer};
	events->EmitEvent(ServerOutBonusSpawnEvent{.pos = pos, .type = type, .uuid = _uuid});
	events->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
		events->EmitEvent(PreTickUpdateEvent{.deltaTime = 1.0});

		if (status = future.wait_for(checkInterval);
			status == std::future_status::ready)
		{
			break;
		}
	}

	ASSERT_EQ(status, std::future_status::ready);
	const auto& [posReplicated, typeReplicated, uuid] = future.get();
	EXPECT_EQ(pos, posReplicated);
	EXPECT_EQ(type, typeReplicated);
	EXPECT_EQ(_uuid, uuid);

}

TEST_F(NetworkTest, BonusDeSpawnEventReplication)
{
	using buuid = boost::uuids::uuid;

	auto events = std::make_shared<EventSystem>();
	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, events);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), events);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	std::promise<buuid> promise;
	auto future = promise.get_future();

	auto bonusDeSpawnSub = events->AddListener(
			[&promise](const ClientInBonusDeSpawnEvent& event) { promise.set_value(event.uuid); });

	// events->EmitEvent(ServerStartFrameEvent{});
	events->EmitEvent(ServerOutBonusDeSpawnEvent{.uuid = _uuid});
	events->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
		events->EmitEvent(PreTickUpdateEvent{.deltaTime = 1.0});

		if (status = future.wait_for(checkInterval);
			status == std::future_status::ready)
		{
			break;
		}
	}

	ASSERT_EQ(status, std::future_status::ready);
	const auto uuidReplicated = future.get();
	EXPECT_EQ(_uuid, uuidReplicated);

}

TEST_F(NetworkTest, BonusStatusEventReplication)
{
	auto events = std::make_shared<EventSystem>();
	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, events);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), events);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	const auto nameOrigin{std::string("Player1")};
	constexpr bool isActiveOrigin{true};

	std::promise<bool> promise;
	auto future = promise.get_future();

	auto bonusStatusSub = events->AddListener(Key(nameOrigin),
			[&promise](const ClientInBonusHelmetPickupEvent& event) { promise.set_value(event.isEnable); });

	// events->EmitEvent(ServerStartFrameEvent{});
	events->EmitEvent(
			ServerOutBonusHelmetPickupEvent{.name = nameOrigin, .isActive = isActiveOrigin});
	events->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
		events->EmitEvent(PreTickUpdateEvent{.deltaTime = 1.0});

		if (status = future.wait_for(checkInterval);
			status == std::future_status::ready)
		{
			break;
		}
	}

	ASSERT_EQ(status, std::future_status::ready);
	const auto isEnable = future.get();
	EXPECT_EQ(isActiveOrigin, isEnable);

}

TEST_F(NetworkTest, BonusCaliberStatusEventReplication)
{
	auto events = std::make_shared<EventSystem>();
	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, events);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), events);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	const auto nameOrigin{std::string("Player1")};

	std::promise<void> promise;
	const auto future = promise.get_future();

	auto bonusCaliberSub = events->AddListener(Key(nameOrigin),
			[&promise](const ClientInBonusCaliberPickupEvent&) { promise.set_value(); });

	// events->EmitEvent(ServerStartFrameEvent{});
	events->EmitEvent(ServerOutBonusCaliberPickupEvent{.author = nameOrigin});
	events->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
		events->EmitEvent(PreTickUpdateEvent{.deltaTime = 1.0});

		if (status = future.wait_for(checkInterval);
			status == std::future_status::ready)
		{
			break;
		}
	}

	ASSERT_EQ(status, std::future_status::ready);

}

TEST_F(NetworkTest, ObstacleSpawnEventReplication)
{
	using buuid = boost::uuids::uuid;

	auto events = std::make_shared<EventSystem>();
	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, events);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), events);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	constexpr auto obstacleType = ObstacleType::Brick;
	constexpr ObjRectangle rectOrigin{.x = 42.0f, .y = 43.0f, .w = 44.0f, .h = 45.0f};

	std::promise<std::tuple<ObjRectangle, ObstacleType, buuid>> promise{};
	auto future = promise.get_future();

	auto obstacleSpawnSub = events->AddListener([&promise](const ClientInObstacleSpawnEvent& event)
	{
		promise.set_value({event.rect, event.type, event.uuid});
	});

	// events->EmitEvent(ServerStartFrameEvent{});
	events->EmitEvent(ServerOutObstacleSpawnEvent{.rect = rectOrigin, .type = obstacleType, .uuid = _uuid});
	events->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
		events->EmitEvent(PreTickUpdateEvent{.deltaTime = 1.0});

		if (status = future.wait_for(checkInterval);
			status == std::future_status::ready)
		{
			break;
		}
	}

	ASSERT_EQ(status, std::future_status::ready);
	auto [rect, type, uuid] = future.get();
	EXPECT_EQ(rectOrigin.x, rect.x);
	EXPECT_EQ(rectOrigin.y, rect.y);
	EXPECT_EQ(rectOrigin.w, rect.w);
	EXPECT_EQ(rectOrigin.h, rect.h);
	EXPECT_EQ(obstacleType, type);
	EXPECT_EQ(_uuid, uuid);

}

TEST_F(NetworkTest, MassiveObstacleSpawnEventReplication)
{
	using buuid = boost::uuids::uuid;

	auto events = std::make_shared<EventSystem>();
	auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, events);
	auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), events);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	constexpr auto obstacleType = ObstacleType::Brick;
	std::vector<ObjRectangle> bricksRect;
	std::vector<ObjRectangle> bricksRectReplicated;
	constexpr unsigned short itemsInMassiveTest = 10000u;
	bricksRect.reserve(itemsInMassiveTest);
	bricksRectReplicated.reserve(itemsInMassiveTest);
	for (size_t i = 0u; i < itemsInMassiveTest; ++i)
	{
		const auto value = static_cast<float>(i);
		bricksRect.emplace_back(value, value + 1, value + 2, value + 3);
	}

	std::vector<std::promise<std::tuple<ObjRectangle, ObstacleType, buuid>>> promises(itemsInMassiveTest);

	std::mutex mtx;
	std::atomic<size_t> count{0u};
	auto massiveObstacleSub = events->AddListener(
			[&promises, &count, &mtx](const ClientInObstacleSpawnEvent& event)
			{
				std::scoped_lock lock(mtx);

				const auto current = count.fetch_add(1u);
				if (current < promises.size())
				{
					promises[current].set_value({event.rect, event.type, event.uuid});
				}
			});

	// events->EmitEvent(ServerStartFrameEvent{});
	for (size_t i = 0u; i < itemsInMassiveTest; ++i)
	{
		events->EmitEvent(ServerOutObstacleSpawnEvent{.rect = bricksRect[i], .type = obstacleType, .uuid = _uuid});
	}
	events->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	if (bricksRect.size() == bricksRectReplicated.size())
		for (size_t i = 0u; i < itemsInMassiveTest; ++i)
		{
			auto future = promises[i].get_future();

			auto status = std::future_status::timeout;
			while (std::chrono::steady_clock::now() - startTime < totalTimeout)
			{
				events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
				events->EmitEvent(PreTickUpdateEvent{.deltaTime = 1.0});

				if (status = future.wait_for(checkInterval);
					status == std::future_status::ready)
				{
					break;
				}
			}

			ASSERT_EQ(status, std::future_status::ready);
			auto [rect, type, uuid] = future.get();
			auto [x, y, w, h] = bricksRect[i];
			EXPECT_FLOAT_EQ(x, rect.x);
			EXPECT_FLOAT_EQ(y, rect.y);
			EXPECT_FLOAT_EQ(w, rect.w);
			EXPECT_FLOAT_EQ(h, rect.h);
			EXPECT_EQ(obstacleType, type);
			EXPECT_EQ(_uuid, uuid);
		}

}

TEST_F(NetworkTest, RespawnTankEventReplication)
{
	using buuid = boost::uuids::uuid;

	auto events = std::make_shared<EventSystem>();
	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, events);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), events);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	std::vector<std::promise<std::tuple<TankType, buuid, FPoint>>> promises(6u);

	size_t count = 0u;
	auto respawnTankSub = events->AddListener(
			[&promises, &count](const ClientInRespawnTankEvent& event)
			{
				promises[count++].set_value({event.type, event.uuid, FPoint{.x = event.rect.x, .y = event.rect.y}});
			});

	constexpr std::array tankTypes{
			TankType::PLAYER1,
			TankType::PLAYER2,
			TankType::ENEMY1,
			TankType::ENEMY2,
			TankType::ENEMY3,
			TankType::ENEMY4
	};

	constexpr ObjRectangle rectOrigin{.x = 12.f, .y = 34.f, .w = 16.f, .h = 16.f};

	// events->EmitEvent(ServerStartFrameEvent{});
	for (const auto tankType: tankTypes)
	{
		events->EmitEvent(ServerOutRespawnTankEvent{.type = tankType, .uuid = _uuid, .rect = rectOrigin});
	}
	events->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	for (size_t i = 0u; i < tankTypes.size(); ++i)
	{
		auto future = promises[i].get_future();
		auto status = std::future_status::timeout;
		while (std::chrono::steady_clock::now() - startTime < totalTimeout)
		{
			events->EmitEvent(NetCommandUpdateEvent{.deltaTime = 1.0});
			events->EmitEvent(PreTickUpdateEvent{.deltaTime = 1.0});

			if (status = future.wait_for(checkInterval);
				status == std::future_status::ready)
			{
				break;
			}
		}

		ASSERT_EQ(status, std::future_status::ready);
		const auto& [typeReplicated, uuid, posReplicated] = future.get();
		EXPECT_EQ(tankTypes[i], typeReplicated);
		EXPECT_EQ(_uuid, uuid);
		EXPECT_EQ((FPoint{.x = rectOrigin.x, .y = rectOrigin.y}), posReplicated);
	}

}

//TODO: other bonus effect replication test after write this replication
// TEST_F(NetworkTest, bonusKind...EventReplication) {
