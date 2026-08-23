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
#include "geometry/ObjRectangle.h"
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
#include <sstream>
#include <chrono>
#include <future>
#include <tuple>
#include <memory>
#include <optional>
#include <thread>
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
	double _deltaTimeOneFrame{1.f / 60.f};

	//NOTE: stands in for MainLoop - received commands sit in a queue until NetCommandUpdate drains it
	void Pump() const
	{
		_hostEvents->EmitEvent(NetCommandUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		_clientEvents->EmitEvent(NetCommandUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		_hostEvents->EmitEvent(PreTickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
		_clientEvents->EmitEvent(PreTickUpdateEvent{.deltaTime = _deltaTimeOneFrame});
	}
};

TEST_F(NetworkTest, PosEventReplication)
{
	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, _hostEvents);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), _clientEvents);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		Pump();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	constexpr FPoint posOrigin{.x = 42.f, .y = 42.f};
	constexpr auto directionOrigin{Direction::UP};

	std::promise<std::tuple<FPoint, Direction>> promise{};
	auto future = promise.get_future();

	const auto name{std::string("TestTank")};
	auto posSub = _clientEvents->AddListener(Key(_uuid), [&promise](const PosChangedEvent& event)
	{
		promise.set_value({event.pos, event.dir});
	});

	_hostEvents->EmitEvent(
			PosChangedEvent{.who = name, .pos = posOrigin, .dir = directionOrigin, .uuid = _uuid});
	_hostEvents->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		Pump();

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

	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, _hostEvents);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), _clientEvents);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		Pump();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	constexpr Direction direction{Direction::UP};

	std::promise<std::pair<Direction, Uuid>> promise{};
	auto future = promise.get_future();

	const auto name{std::string("TestTank")};
	auto shotSub = _clientEvents->AddListener(Key(name), [&promise](const TankShotEvent& event)
	{
		promise.set_value({event.dir, event.bulletUuid});
	});

	_hostEvents->EmitEvent(TankShotEvent{.who = name, .dir = direction, .bulletUuid = _uuid});
	_hostEvents->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		Pump();

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
	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, _hostEvents);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), _clientEvents);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		Pump();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	constexpr int healthOrigin{42};

	std::promise<int> promise{};
	auto future = promise.get_future();

	const auto name{std::string("TestTank")};

	auto healthSub = _clientEvents->AddListener(Key(_uuid),
			[&promise](const HealthChangedEvent& event) { promise.set_value(event.health); });

	_hostEvents->EmitEvent(HealthChangedEvent{.who = name, .health = healthOrigin, .uuid = _uuid});
	_hostEvents->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		Pump();

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

TEST_F(NetworkTest, DespawnEventReplication)
{

	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, _hostEvents);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), _clientEvents);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		Pump();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	std::promise<Uuid> promise{};
	auto future = promise.get_future();

	auto despawnSub = _clientEvents->AddListener(
			Key(_uuid), [&promise, uuid = _uuid](const DespawnedEvent&) { promise.set_value(uuid); });

	_hostEvents->EmitEvent(DespawnedEvent{.who = "Bullet", .uuid = _uuid, .reason = DespawnReason::Destroyed});
	_hostEvents->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		Pump();

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
	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, _hostEvents);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), _clientEvents);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		Pump();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	std::promise<std::pair<std::string, std::string>> promise{};
	auto future = promise.get_future();

	auto statsSub = _clientEvents->AddListener([&promise](const StatisticsBulletHitEvent& event)
	{
		promise.set_value({event.author, event.fraction});
	});

	_hostEvents->EmitEvent(StatisticsBulletHitEvent{.author = "author", .fraction = "fraction"});
	_hostEvents->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		Pump();

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

TEST_F(NetworkTest, PauseRequestFromClientPausesHost)
{
	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, _hostEvents);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), _clientEvents);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		Pump();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	std::promise<void> promise{};
	auto future = promise.get_future();

	auto pauseSub = _hostEvents->AddListener([&promise](const PauseReleasedEvent&) { promise.set_value(); });

	_clientEvents->EmitEvent(PauseRequestedEvent{.isPaused = true});
	_clientEvents->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		Pump();

		if (status = future.wait_for(checkInterval);
			status == std::future_status::ready)
		{
			break;
		}
	}

	ASSERT_EQ(status, std::future_status::ready);
}

TEST_F(NetworkTest, BonusSpawnEventReplication)
{

	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, _hostEvents);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), _clientEvents);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		Pump();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	std::promise<std::tuple<FPoint, BonusType, Uuid>> promise{};
	auto future = promise.get_future();

	auto bonusSpawnSub = _clientEvents->AddListener([&promise](const BonusSpawnedEvent& event)
	{
		promise.set_value({event.pos, event.type, event.uuid});
	});

	constexpr FPoint pos{.x = 42.f, .y = 42.f};
	constexpr auto type{BonusType::Timer};
	_hostEvents->EmitEvent(BonusSpawnedEvent{.pos = pos, .type = type, .uuid = _uuid});
	_hostEvents->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		Pump();

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

TEST_F(NetworkTest, DespawnReasonReplication)
{

	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, _hostEvents);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), _clientEvents);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		Pump();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	std::promise<DespawnReason> promise;
	auto future = promise.get_future();

	auto despawnSub = _clientEvents->AddListener(
			Key(_uuid), [&promise](const DespawnedEvent& event) { promise.set_value(event.reason); });

	_hostEvents->EmitEvent(
			DespawnedEvent{.who = "BonusHelmet", .uuid = _uuid, .reason = DespawnReason::PickedUp});
	_hostEvents->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		Pump();

		if (status = future.wait_for(checkInterval);
			status == std::future_status::ready)
		{
			break;
		}
	}

	ASSERT_EQ(status, std::future_status::ready);
	EXPECT_EQ(DespawnReason::PickedUp, future.get());

}

TEST_F(NetworkTest, BonusStatusEventReplication)
{
	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, _hostEvents);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), _clientEvents);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		Pump();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	const auto nameOrigin{std::string("Player1")};
	constexpr bool isActiveOrigin{true};

	std::promise<bool> promise;
	auto future = promise.get_future();

	auto bonusStatusSub = _clientEvents->AddListener(Key(nameOrigin),
			[&promise](const BonusHelmetAppliedEvent& event) { promise.set_value(event.isActive); });

	_hostEvents->EmitEvent(
			BonusHelmetAppliedEvent{.name = nameOrigin, .isActive = isActiveOrigin});
	_hostEvents->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		Pump();

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
	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, _hostEvents);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), _clientEvents);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		Pump();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	const auto nameOrigin{std::string("Player1")};

	std::promise<void> promise;
	const auto future = promise.get_future();

	auto bonusCaliberSub = _clientEvents->AddListener(Key(nameOrigin),
			[&promise](const BonusCaliberAppliedEvent&) { promise.set_value(); });

	_hostEvents->EmitEvent(BonusCaliberAppliedEvent{.name = nameOrigin});
	_hostEvents->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		Pump();

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

	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, _hostEvents);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), _clientEvents);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		Pump();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	constexpr auto obstacleType = ObstacleType::Brick;
	constexpr FPoint posOrigin{.x = 42.0f, .y = 43.0f};

	std::promise<std::tuple<FPoint, ObstacleType, Uuid>> promise{};
	auto future = promise.get_future();

	auto obstacleSpawnSub = _clientEvents->AddListener([&promise](const ObstacleSpawnedEvent& event)
	{
		promise.set_value({event.pos, event.type, event.uuid});
	});

	_hostEvents->EmitEvent(ObstacleSpawnedEvent{.pos = posOrigin, .type = obstacleType, .uuid = _uuid});
	_hostEvents->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	auto status = std::future_status::timeout;
	while (std::chrono::steady_clock::now() - startTime < totalTimeout)
	{
		Pump();

		if (status = future.wait_for(checkInterval);
			status == std::future_status::ready)
		{
			break;
		}
	}

	ASSERT_EQ(status, std::future_status::ready);
	auto [pos, type, uuid] = future.get();
	EXPECT_EQ(posOrigin.x, pos.x);
	EXPECT_EQ(posOrigin.y, pos.y);
	EXPECT_EQ(obstacleType, type);
	EXPECT_EQ(_uuid, uuid);

}

TEST_F(NetworkTest, MassiveObstacleSpawnEventReplication)
{

	auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, _hostEvents);
	auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), _clientEvents);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		Pump();
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

	std::vector<std::promise<std::tuple<FPoint, ObstacleType, Uuid>>> promises(itemsInMassiveTest);

	std::mutex mtx;
	std::atomic<size_t> count{0u};
	auto massiveObstacleSub = _clientEvents->AddListener(
			[&promises, &count, &mtx](const ObstacleSpawnedEvent& event)
			{
				std::scoped_lock lock(mtx);

				const auto current = count.fetch_add(1u);
				if (current < promises.size())
				{
					promises[current].set_value({event.pos, event.type, event.uuid});
				}
			});

	for (size_t i = 0u; i < itemsInMassiveTest; ++i)
	{
		_hostEvents->EmitEvent(ObstacleSpawnedEvent{
				.pos = FPoint{.x = bricksRect[i].x, .y = bricksRect[i].y}, .type = obstacleType, .uuid = _uuid});
	}
	_hostEvents->EmitEvent(NetworkEndFrameEvent{});

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
				Pump();

				if (status = future.wait_for(checkInterval);
					status == std::future_status::ready)
				{
					break;
				}
			}

			ASSERT_EQ(status, std::future_status::ready);
			auto [pos, type, uuid] = future.get();
			auto [x, y, w, h] = bricksRect[i];
			EXPECT_FLOAT_EQ(x, pos.x);
			EXPECT_FLOAT_EQ(y, pos.y);
			EXPECT_EQ(obstacleType, type);
			EXPECT_EQ(_uuid, uuid);
		}

}

TEST_F(NetworkTest, RespawnTankEventReplication)
{

	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, _hostEvents);
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), _clientEvents);

	constexpr std::chrono::milliseconds connectTimeout{5000};
	const auto connectStart = std::chrono::steady_clock::now();
	while (!client->IsConnected() && std::chrono::steady_clock::now() - connectStart < connectTimeout)
	{
		Pump();
		std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	ASSERT_TRUE(client->IsConnected());

	std::vector<std::promise<std::tuple<TankType, Uuid, FPoint>>> promises(6u);

	size_t count = 0u;
	auto respawnTankSub = _clientEvents->AddListener(
			[&promises, &count](const TankRespawnedEvent& event)
			{
				promises[count++].set_value({event.type, event.uuid, event.pos});
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

	for (const auto tankType: tankTypes)
	{
		_hostEvents->EmitEvent(TankRespawnedEvent{
				.type = tankType, .uuid = _uuid, .pos = FPoint{.x = rectOrigin.x, .y = rectOrigin.y}});
	}
	_hostEvents->EmitEvent(NetworkEndFrameEvent{});

	constexpr std::chrono::milliseconds totalTimeout{5000};
	constexpr std::chrono::milliseconds checkInterval{1};
	const std::chrono::steady_clock::time_point startTime = std::chrono::steady_clock::now();
	for (size_t i = 0u; i < tankTypes.size(); ++i)
	{
		auto future = promises[i].get_future();
		auto status = std::future_status::timeout;
		while (std::chrono::steady_clock::now() - startTime < totalTimeout)
		{
			Pump();

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

//NOTE: exercises the link itself, not a command traveling over it
TEST_F(NetworkTest, ClientReconnectsAfterEstablishedLinkDrops)
{
	auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, _hostEvents);
	const uint16_t port = server->GetBoundPort();
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", port, _clientEvents);

	//NOTE: the network runs on its own threads; this just drives the game-side events MainLoop would
	const auto pumpUntil = [this](auto&& predicate, const std::chrono::milliseconds timeout)
	{
		const auto start = std::chrono::steady_clock::now();
		while (!predicate() && std::chrono::steady_clock::now() - start < timeout)
		{
			Pump();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		return predicate();
	};

	ASSERT_TRUE(pumpUntil([&client] { return client->IsConnected(); }, std::chrono::milliseconds{5000}));

	//NOTE: Abort, not just reset - an announced leave stops the reconnect, and this test wants one
	server->Abort();
	server.reset();

	ASSERT_TRUE(pumpUntil([&client] { return !client->IsConnected(); }, std::chrono::milliseconds{5000}))
			<< "client never noticed the link dropped";

	//NOTE: same port - the client keeps its endpoint; retried, the old listener may still hold it.
	//The budget is deliberately short: the client gives up after MaxReconnectAttempts * ReconnectDelayMs
	//(~5s), so a slow re-bind would eat the very window this test is checking.
	const bool reBound = pumpUntil([&]
	{
		if (!server)
		{
			try
			{
				server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", port, _hostEvents);
			}
			catch (const std::exception&)
			{
				return false;
			}
		}
		return true;
	}, std::chrono::milliseconds{1500});

	if (!reBound)
	{
		GTEST_SKIP() << "port " << port << " still held by the OS - nothing to test against";
	}

	EXPECT_TRUE(pumpUntil([&client] { return client->IsConnected(); }, std::chrono::milliseconds{10000}))
			<< "client did not reconnect after the host came back";
}

//NOTE: like the reconnection test above - about the link, not about a command riding it
TEST_F(NetworkTest, HostShutdownTellsClientWhyAndStopsTheReconnect)
{
	auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, _hostEvents);
	const uint16_t port = server->GetBoundPort();
	const auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", port, _clientEvents);

	const auto pumpUntil = [this](auto&& predicate, const std::chrono::milliseconds timeout)
	{
		const auto start = std::chrono::steady_clock::now();
		while (!predicate() && std::chrono::steady_clock::now() - start < timeout)
		{
			Pump();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		return predicate();
	};

	ASSERT_TRUE(pumpUntil([&client] { return client->IsConnected(); }, std::chrono::milliseconds{5000}));

	std::optional<DisconnectReason> received{};
	auto disconnectSub = _clientEvents->AddListener([&received](const ClientInDisconnectEvent& event)
	{
		received = event.reason;
	});

	server.reset();//NOTE: the goodbye goes out from inside the destructor, before the socket closes

	ASSERT_TRUE(pumpUntil([&received] { return received.has_value(); }, std::chrono::milliseconds{5000}))
			<< "client never got the host's goodbye";
	EXPECT_EQ(DisconnectReason::HostShutdown, *received);

	//NOTE: the point of the reason - a plain drop would have the client retrying this very port
	const bool reBound = pumpUntil([&]
	{
		if (!server)
		{
			try
			{
				server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", port, _hostEvents);
			}
			catch (const std::exception&)
			{
				return false;
			}
		}
		return true;
	}, std::chrono::milliseconds{1500});

	if (!reBound)
	{
		GTEST_SKIP() << "port " << port << " still held by the OS - nothing to test against";
	}

	//NOTE: one retry period is enough - the timer has been running since the drop, so the next
	//attempt lands here. The ~5s budget says when the client stops trying, not when it starts.
	EXPECT_FALSE(pumpUntil([&client] { return client->IsConnected(); }, std::chrono::milliseconds{1500}))
			<< "client reconnected after the host said it was leaving on purpose";
}

TEST_F(NetworkTest, ClientQuitTellsHostWhy)
{
	const auto server = std::make_unique<network::commands::ServerHandler>("127.0.0.1", 0, _hostEvents);
	auto client = std::make_unique<network::commands::ClientHandler>("127.0.0.1", server->GetBoundPort(), _clientEvents);

	const auto pumpUntil = [this](auto&& predicate, const std::chrono::milliseconds timeout)
	{
		const auto start = std::chrono::steady_clock::now();
		while (!predicate() && std::chrono::steady_clock::now() - start < timeout)
		{
			Pump();
			std::this_thread::sleep_for(std::chrono::milliseconds(1));
		}
		return predicate();
	};

	ASSERT_TRUE(pumpUntil([&client] { return client->IsConnected(); }, std::chrono::milliseconds{5000}));

	std::optional<DisconnectReason> received{};
	auto disconnectSub = _hostEvents->AddListener([&received](const ServerInDisconnectEvent& event)
	{
		received = event.reason;
	});

	client.reset();

	ASSERT_TRUE(pumpUntil([&received] { return received.has_value(); }, std::chrono::milliseconds{5000}))
			<< "host never got the client's goodbye";
	EXPECT_EQ(DisconnectReason::PlayerQuit, *received);
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
