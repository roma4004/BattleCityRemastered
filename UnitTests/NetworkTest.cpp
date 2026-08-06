#include "Point.h"
#include "components/EventSystem.h"
#include "components/SpawnEvents.h"
#include "components/events/ObstacleAndBonusEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/StatisticsEvents.h"
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
	using buuid = boost::uuids::uuid;

	auto events = std::make_shared<EventSystem>();
	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, events);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), events);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		events->EmitEvent("NetCommandUpdate", 1.0);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	constexpr FPoint posOrigin{.x = 42.f, .y = 42.f};
	constexpr auto directionOrigin{Direction::UP};

	std::promise<std::tuple<FPoint, Direction>> promise{};
	auto future = promise.get_future();

	const auto name{std::string("TestTank")};
	events->AddListener(
			"ClientReceived_Pos", _uuid, "PosEventReplication",
			[&promise](const ClientReceivedPosEvent& event)
			{
				promise.set_value({event.pos, event.dir});
			});

	// events->EmitEvent("Server_StartFrame");
	events->EmitEvent("ServerSend_Pos",
					  ServerSendPosEvent{.who = name, .pos = posOrigin, .dir = directionOrigin, .uuid = _uuid});
	events->EmitEvent("Server_EndFrame");

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		events->EmitEvent("NetCommandUpdate", 1.0);
		events->EmitEvent("PreTickUpdate", 1.0);

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

	events->RemoveAllListeners("PosEventReplication");
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
		events->EmitEvent("NetCommandUpdate", 1.0);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	constexpr Direction direction{Direction::UP};

	std::promise<std::pair<Direction, buuid>> promise{};
	auto future = promise.get_future();

	const auto name{std::string("TestTank")};
	events->AddListener("ClientReceived_Shot", name, "ShotEventReplication",
						[&promise](const ClientReceivedShotEvent& event)
						{
							promise.set_value({event.dir, event.bulletUuid});
						});

	// events->EmitEvent("Server_StartFrame");
	events->EmitEvent("ServerSend_Shot", ServerSendShotEvent{.who = name, .dir = direction, .bulletUuid = _uuid});
	events->EmitEvent("Server_EndFrame");

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		events->EmitEvent("NetCommandUpdate", 1.0);
		events->EmitEvent("PreTickUpdate", 1.0);

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

	events->RemoveAllListeners("ShotEventReplication");
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
		events->EmitEvent("NetCommandUpdate", 1.0);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	constexpr int healthOrigin{42};

	std::promise<int> promise{};
	auto future = promise.get_future();

	const auto name{std::string("TestTank")};

	events->AddListener("ClientReceived_Health", _uuid, "HealthEventReplication",
						[&promise](const int health) { promise.set_value(health); });

	// events->EmitEvent("Server_StartFrame");
	events->EmitEvent("ServerSend_Health", ServerSendHealthEvent{.who = name, .health = healthOrigin, .uuid = _uuid});
	events->EmitEvent("Server_EndFrame");

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		events->EmitEvent("NetCommandUpdate", 1.0);
		events->EmitEvent("PreTickUpdate", 1.0);

		if (status = future.wait_for(checkInterval);
			status == std::future_status::ready)
		{
			break;
		}
	}

	ASSERT_EQ(status, std::future_status::ready);
	const auto healthReplicated = future.get();
	EXPECT_EQ(healthOrigin, healthReplicated);

	events->RemoveAllListeners("HealthEventReplication");
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
		events->EmitEvent("NetCommandUpdate", 1.0);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	std::promise<buuid> promise{};
	auto future = promise.get_future();

	events->AddListener("ClientReceived_Dispose", _uuid, "DisposeEventReplication",
						[&promise, uuid = _uuid]() { promise.set_value(uuid); });

	// events->EmitEvent("Server_StartFrame");
	events->EmitEvent("ServerSend_Dispose", _uuid);
	events->EmitEvent("Server_EndFrame");

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		events->EmitEvent("NetCommandUpdate", 1.0);
		events->EmitEvent("PreTickUpdate", 1.0);

		if (status = future.wait_for(checkInterval);
			status == std::future_status::ready)
		{
			break;
		}
	}

	ASSERT_EQ(status, std::future_status::ready);
	const auto uuidReplicated = future.get();
	EXPECT_EQ(_uuid, uuidReplicated);

	events->RemoveAllListeners("DisposeEventReplication");
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
		events->EmitEvent("NetCommandUpdate", 1.0);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	std::promise<std::tuple<std::string, std::string, std::string>> promise{};
	auto future = promise.get_future();

	events->AddListener(
			"ClientReceived_Statistics", "StatisticsEventReplication",
			[&promise](const ClientReceivedStatisticsEvent& event)
			{
				promise.set_value({event.eventName, event.author, event.fraction});
			});

	// events->EmitEvent("Server_StartFrame");
	events->EmitEvent("ServerSend_Statistics",
					  ServerSendStatisticsEvent{.eventName = "BulletHit", .author = "author", .fraction = "fraction"});
	events->EmitEvent("Server_EndFrame");

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		events->EmitEvent("NetCommandUpdate", 1.0);
		events->EmitEvent("PreTickUpdate", 1.0);

		if (status = future.wait_for(checkInterval);
			status == std::future_status::ready)
		{
			break;
		}
	}

	ASSERT_EQ(status, std::future_status::ready);
	const auto& [type, author, fraction] = future.get();
	EXPECT_EQ("BulletHit", type);
	EXPECT_EQ("author", author);
	EXPECT_EQ("fraction", fraction);

	events->RemoveListener("ClientReceived_Statistics", "StatisticsEventReplication");
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
		events->EmitEvent("NetCommandUpdate", 1.0);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	std::promise<std::pair<std::string, buuid>> promiseDied1{};
	auto futureDied1 = promiseDied1.get_future();
	std::promise<std::pair<std::string, buuid>> promiseDied2{};
	auto futureDied2 = promiseDied2.get_future();

	std::promise<std::pair<std::string, buuid>> promiseToBrick1{};
	auto futureToBrick1 = promiseToBrick1.get_future();
	std::promise<std::pair<std::string, buuid>> promiseToBrick2{};
	auto futureToBrick2 = promiseToBrick2.get_future();

	std::promise<std::pair<std::string, buuid>> promiseToSteel1{};
	auto futureToSteel1 = promiseToSteel1.get_future();
	std::promise<std::pair<std::string, buuid>> promiseToSteel2{};
	auto futureToSteel2 = promiseToSteel2.get_future();

	events->AddListener(
			"ClientReceived_FortressChange", "FortressChangeEventReplication1",
			[&promiseDied1, &promiseToBrick1, &promiseToSteel1, &uuid1Died, &uuid1ToBrick, &uuid1ToSteel](
			const FortressChangeEvent& event)
			{
				if (event.state == "Died" && event.uuid == uuid1Died)
				{
					promiseDied1.set_value({event.state, event.uuid});
				}
				else if (event.state == "ToBrick" && event.uuid == uuid1ToBrick)
				{
					promiseToBrick1.set_value({event.state, event.uuid});
				}
				else if (event.state == "ToSteel" && event.uuid == uuid1ToSteel)
				{
					promiseToSteel1.set_value({event.state, event.uuid});
				}
			});
	events->AddListener(
			"ClientReceived_FortressChange", "FortressChangeEventReplication2",
			[&promiseDied2, &promiseToBrick2, &promiseToSteel2, &uuid2Died, &uuid2ToBrick, &uuid2ToSteel](
			const FortressChangeEvent& event)
			{
				if (event.state == "Died" && event.uuid == uuid2Died)
				{
					promiseDied2.set_value({event.state, event.uuid});
				}
				else if (event.state == "ToBrick" && event.uuid == uuid2ToBrick)
				{
					promiseToBrick2.set_value({event.state, event.uuid});
				}
				else if (event.state == "ToSteel" && event.uuid == uuid2ToSteel)
				{
					promiseToSteel2.set_value({event.state, event.uuid});
				}
			});

	// events->EmitEvent("Server_StartFrame");
	events->EmitEvent("ServerSend_FortressChange", FortressChangeEvent{.state = "Died", .uuid = uuid1Died});
	events->EmitEvent("ServerSend_FortressChange", FortressChangeEvent{.state = "ToBrick", .uuid = uuid1ToBrick});
	events->EmitEvent("ServerSend_FortressChange", FortressChangeEvent{.state = "ToSteel", .uuid = uuid1ToSteel});

	events->EmitEvent("ServerSend_FortressChange", FortressChangeEvent{.state = "Died", .uuid = uuid2Died});
	events->EmitEvent("ServerSend_FortressChange", FortressChangeEvent{.state = "ToBrick", .uuid = uuid2ToBrick});
	events->EmitEvent("ServerSend_FortressChange", FortressChangeEvent{.state = "ToSteel", .uuid = uuid2ToSteel});

	events->EmitEvent("Server_EndFrame");

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();

	{//test first fortressWall
		{//died1 test
			auto statusToDied1 = std::future_status::timeout;
			while (std::chrono::steady_clock::now() - startTime < totalTimeout)
			{
				events->EmitEvent("NetCommandUpdate", 1.0);
				events->EmitEvent("PreTickUpdate", 1.0);

				if (statusToDied1 = futureDied1.wait_for(checkInterval);
					statusToDied1 == std::future_status::ready)
				{
					break;
				}
			}

			ASSERT_EQ(statusToDied1, std::future_status::ready);
			const auto& [stateDied, uuidDied] = futureDied1.get();
			EXPECT_EQ("Died", stateDied);
			EXPECT_EQ(uuid1Died, uuidDied);
		}
		{//brick1 test
			auto statusToBrick1 = std::future_status::timeout;
			while (std::chrono::steady_clock::now() - startTime < totalTimeout)
			{
				events->EmitEvent("NetCommandUpdate", 1.0);
				events->EmitEvent("PreTickUpdate", 1.0);

				if (statusToBrick1 = futureToBrick1.wait_for(checkInterval);
					statusToBrick1 == std::future_status::ready)
				{
					break;
				}
			}

			ASSERT_EQ(statusToBrick1, std::future_status::ready);
			const auto& [stateToBrick, uuidToBrick] = futureToBrick1.get();
			EXPECT_EQ("ToBrick", stateToBrick);
			EXPECT_EQ(uuid1ToBrick, uuidToBrick);
		}
		{//steel1 test
			auto statusToSteel1 = std::future_status::timeout;
			while (std::chrono::steady_clock::now() - startTime < totalTimeout)
			{
				events->EmitEvent("NetCommandUpdate", 1.0);
				events->EmitEvent("PreTickUpdate", 1.0);

				if (statusToSteel1 = futureToSteel1.wait_for(checkInterval);
					statusToSteel1 == std::future_status::ready)
				{
					break;
				}
			}

			ASSERT_EQ(statusToSteel1, std::future_status::ready);
			const auto& [stateToSteel, uuidToSteel] = futureToSteel1.get();
			EXPECT_EQ("ToSteel", stateToSteel);
			EXPECT_EQ(uuid1ToSteel, uuidToSteel);
		}
	}
	{//test second fortressWall
		{//died2 test
			auto statusToDied2 = std::future_status::timeout;
			while (std::chrono::steady_clock::now() - startTime < totalTimeout)
			{
				events->EmitEvent("NetCommandUpdate", 1.0);
				events->EmitEvent("PreTickUpdate", 1.0);

				if (statusToDied2 = futureDied2.wait_for(checkInterval);
					statusToDied2 == std::future_status::ready)
				{
					break;
				}
			}

			ASSERT_EQ(statusToDied2, std::future_status::ready);
			const auto& [stateDied, uuidDied] = futureDied2.get();
			EXPECT_EQ("Died", stateDied);
			EXPECT_EQ(uuid2Died, uuidDied);
		}
		{//brick2 test
			auto statusToBrick2 = std::future_status::timeout;
			while (std::chrono::steady_clock::now() - startTime < totalTimeout)
			{
				events->EmitEvent("NetCommandUpdate", 1.0);
				events->EmitEvent("PreTickUpdate", 1.0);

				if (statusToBrick2 = futureToBrick2.wait_for(checkInterval);
					statusToBrick2 == std::future_status::ready)
				{
					break;
				}
			}

			ASSERT_EQ(statusToBrick2, std::future_status::ready);
			const auto& [stateToBrick, uuidToBrick] = futureToBrick2.get();
			EXPECT_EQ("ToBrick", stateToBrick);
			EXPECT_EQ(uuid2ToBrick, uuidToBrick);
		}
		{//steel2 test
			auto statusToSteel2 = std::future_status::timeout;
			while (std::chrono::steady_clock::now() - startTime < totalTimeout)
			{
				events->EmitEvent("NetCommandUpdate", 1.0);
				events->EmitEvent("PreTickUpdate", 1.0);

				if (statusToSteel2 = futureToSteel2.wait_for(checkInterval);
					statusToSteel2 == std::future_status::ready)
				{
					break;
				}
			}

			ASSERT_EQ(statusToSteel2, std::future_status::ready);
			const auto& [stateToSteel, uuidToSteel] = futureToSteel2.get();
			EXPECT_EQ("ToSteel", stateToSteel);
			EXPECT_EQ(uuid2ToSteel, uuidToSteel);
		}
	}

	events->RemoveListener("ClientReceived_FortressChange", "FortressChangeEventReplication1");
	events->RemoveListener("ClientReceived_FortressChange", "FortressChangeEventReplication2");
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
		events->EmitEvent("NetCommandUpdate", 1.0);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	std::promise<std::tuple<FPoint, BonusType, buuid>> promise{};
	auto future = promise.get_future();

	events->AddListener(
			"ClientReceived_BonusSpawn", "BonusSpawnEventReplication",
			[&promise](const ClientReceivedBonusSpawnEvent& event)
			{
				promise.set_value({event.pos, event.type, event.uuid});
			});

	// events->EmitEvent("Server_StartFrame");
	constexpr FPoint pos{.x = 42.f, .y = 42.f};
	constexpr auto type{BonusType::Timer};
	events->EmitEvent("ServerSend_BonusSpawn", BonusSpawnEvent{.pos = pos, .type = type, .uuid = _uuid});
	events->EmitEvent("Server_EndFrame");

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		events->EmitEvent("NetCommandUpdate", 1.0);
		events->EmitEvent("PreTickUpdate", 1.0);

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

	events->RemoveListener("ClientReceived_BonusSpawn", "BonusSpawnEventReplication");
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
		events->EmitEvent("NetCommandUpdate", 1.0);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	std::promise<buuid> promise;
	auto future = promise.get_future();

	events->AddListener(
			"ClientReceived_BonusDeSpawn", "BonusDeSpawnEventReplication",
			[&promise](const buuid& uuid) { promise.set_value(uuid); });

	// events->EmitEvent("Server_StartFrame");
	events->EmitEvent("ServerSend_BonusDeSpawn", _uuid);
	events->EmitEvent("Server_EndFrame");

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		events->EmitEvent("NetCommandUpdate", 1.0);
		events->EmitEvent("PreTickUpdate", 1.0);

		if (status = future.wait_for(checkInterval);
			status == std::future_status::ready)
		{
			break;
		}
	}

	ASSERT_EQ(status, std::future_status::ready);
	const auto uuidReplicated = future.get();
	EXPECT_EQ(_uuid, uuidReplicated);

	events->RemoveListener("ClientReceived_BonusDeSpawn", "BonusDeSpawnEventReplication");
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
		events->EmitEvent("NetCommandUpdate", 1.0);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	const auto nameOrigin{std::string("Player1")};
	constexpr bool isActiveOrigin{true};

	std::promise<bool> promise;
	auto future = promise.get_future();

	events->AddListener(
			"ClientReceived_BonusHelmet_Pickup", nameOrigin, "BonusStatusEventReplication",
			[&promise](const bool isEnable) { promise.set_value(isEnable); });

	// events->EmitEvent("Server_StartFrame");
	events->EmitEvent("ServerSend_BonusHelmet_Pickup",
					  ServerSendBonusHelmetPickupEvent{.name = nameOrigin, .isActive = isActiveOrigin});
	events->EmitEvent("Server_EndFrame");

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		events->EmitEvent("NetCommandUpdate", 1.0);
		events->EmitEvent("PreTickUpdate", 1.0);

		if (status = future.wait_for(checkInterval);
			status == std::future_status::ready)
		{
			break;
		}
	}

	ASSERT_EQ(status, std::future_status::ready);
	const auto isEnable = future.get();
	EXPECT_EQ(isActiveOrigin, isEnable);

	events->RemoveAllListeners("BonusStatusEventReplication");
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
		events->EmitEvent("NetCommandUpdate", 1.0);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	const auto nameOrigin{std::string("Player1")};

	std::promise<void> promise;
	auto future = promise.get_future();

	events->AddListener(
			"ClientReceived_BonusCaliber_Pickup", nameOrigin, "BonusCaliberStatusEventReplication",
			[&promise]() { promise.set_value(); });

	// events->EmitEvent("Server_StartFrame");
	events->EmitEvent("ServerSend_BonusCaliber_Pickup", nameOrigin);
	events->EmitEvent("Server_EndFrame");

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		events->EmitEvent("NetCommandUpdate", 1.0);
		events->EmitEvent("PreTickUpdate", 1.0);

		if (status = future.wait_for(checkInterval);
			status == std::future_status::ready)
		{
			break;
		}
	}

	ASSERT_EQ(status, std::future_status::ready);

	events->RemoveAllListeners("BonusCaliberStatusEventReplication");
}

TEST_F(NetworkTest, ObstacleSpawnEventReplication)
{
	using buuid = boost::uuids::uuid;

	auto events = std::make_shared<EventSystem>();
	auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, events);
	auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), events);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		events->EmitEvent("NetCommandUpdate", 1.0);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	constexpr auto obstacleType = ObstacleType::Brick;
	constexpr ObjRectangle rectOrigin{.x = 42.0f, .y = 43.0f, .w = 44.0f, .h = 45.0f};

	std::promise<std::tuple<ObjRectangle, ObstacleType, buuid>> promise{};
	auto future = promise.get_future();

	events->AddListener(
			"ClientReceived_ObstacleSpawn", "ObstacleSpawnEventReplication",
			[&promise](const ClientReceivedObstacleSpawnEvent& event)
			{
				promise.set_value({event.rect, event.type, event.uuid});
			});

	// events->EmitEvent("Server_StartFrame");
	events->EmitEvent("ServerSend_ObstacleSpawn",
					  ObstacleSpawnEvent{.rect = rectOrigin, .type = obstacleType, .uuid = _uuid});
	events->EmitEvent("Server_EndFrame");

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		events->EmitEvent("NetCommandUpdate", 1.0);
		events->EmitEvent("PreTickUpdate", 1.0);

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

	events->RemoveListener("ClientReceived_ObstacleSpawn", "ObstacleSpawnEventReplication");
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
		events->EmitEvent("NetCommandUpdate", 1.0);
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
	events->AddListener(
			"ClientReceived_ObstacleSpawn", "MassiveObstacleSpawnEventReplication",
			[&promises, &count, &mtx](const ClientReceivedObstacleSpawnEvent& event)
			{
				std::scoped_lock lock(mtx);

				const auto current = count.fetch_add(1u);
				if (current < promises.size())
				{
					promises[current].set_value({event.rect, event.type, event.uuid});
				}
			});

	// events->EmitEvent("Server_StartFrame");
	for (size_t i = 0u; i < itemsInMassiveTest; ++i)
	{
		events->EmitEvent("ServerSend_ObstacleSpawn",
						  ObstacleSpawnEvent{.rect = bricksRect[i], .type = obstacleType, .uuid = _uuid});
	}
	events->EmitEvent("Server_EndFrame");

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
				events->EmitEvent("NetCommandUpdate", 1.0);
				events->EmitEvent("PreTickUpdate", 1.0);

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

	events->RemoveListener("ClientReceived_ObstacleSpawn", "MassiveObstacleSpawnEventReplication");
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
		events->EmitEvent("NetCommandUpdate", 1.0);
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	std::vector<std::promise<std::tuple<TankType, buuid, FPoint>>> promises(6u);

	size_t count = 0u;
	events->AddListener(
			"ClientReceived_RespawnTank", "RespawnTankEventReplication",
			[&promises, &count](const ClientReceivedRespawnTankEvent& event)
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

	// events->EmitEvent("Server_StartFrame");
	for (const auto tankType: tankTypes)
	{
		events->EmitEvent("ServerSend_RespawnTank",
						  ServerSendRespawnTankEvent{.type = tankType, .uuid = _uuid, .rect = rectOrigin});
	}
	events->EmitEvent("Server_EndFrame");

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	for (size_t i = 0u; i < tankTypes.size(); ++i)
	{
		auto future = promises[i].get_future();
		auto status = std::future_status::timeout;
		while (std::chrono::steady_clock::now() - startTime < totalTimeout)
		{
			events->EmitEvent("NetCommandUpdate", 1.0);
			events->EmitEvent("PreTickUpdate", 1.0);

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

	events->RemoveListener("ClientReceived_RespawnTank", "RespawnTankEventReplication");
}

//TODO: other bonus effect replication test after write this replication
// TEST_F(NetworkTest, bonusKind...EventReplication) {
