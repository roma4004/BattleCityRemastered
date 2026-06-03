#include "Point.h"
#include "components/EventSystem.h"
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

//TODO: fix "DoRead error ...asio.misc:2" in console after each test
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
	auto server = std::make_unique<network::commands::ServerHandler>(events);
	auto client = std::make_unique<network::commands::ClientHandler>(events);

	constexpr FPoint posOrigin{.x = 42.f, .y = 42.f};
	constexpr auto directionOrigin{Direction::UP};

	std::promise<std::tuple<FPoint, Direction, buuid>> promise{};
	auto future = promise.get_future();

	const auto name{std::string("TestTank")};
	events->AddListener(
			"ClientReceived_" + name + "Pos", "PosEventReplication",
			[&promise](const FPoint newPos, const Direction dir, const buuid& uuid)
			{
				promise.set_value({newPos, dir, uuid});
			});

	// events->EmitEvent("Server_StartFrame");
	events->EmitEvent("ServerSend_Pos", name, posOrigin, directionOrigin, _uuid);
	events->EmitEvent("Server_EndFrame");

	const auto status = future.wait_for(std::chrono::milliseconds(1000));
	events->EmitEvent("NetCommandUpdate", 1.0);
	events->EmitEvent("PreTickUpdate", 1.0);
	// ASSERT_EQ(status, std::future_status::ready);

	// ASSERT_EQ(future.valid(), true);

	// ASSERT_EQ(future._Is_ready(), true);

	const auto& [posReplicated, dirReplicated, uuidReplicated] = future.get();

	EXPECT_EQ(posOrigin, posReplicated);
	EXPECT_EQ(directionOrigin, dirReplicated);
	EXPECT_EQ(_uuid, uuidReplicated);

	events->RemoveListener("ClientReceived_" + name + "Pos", "PosEventReplication");
}

TEST_F(NetworkTest, ShotEventReplication)
{
	using buuid = boost::uuids::uuid;

	auto events = std::make_shared<EventSystem>();
	auto server = std::make_unique<network::commands::ServerHandler>(events);
	auto client = std::make_unique<network::commands::ClientHandler>(events);

	constexpr Direction direction{Direction::UP};

	std::promise<std::pair<Direction, buuid>> promise{};
	auto future = promise.get_future();

	const auto name{std::string("TestTank")};
	events->AddListener("ClientReceived_" + name + "Shot", "ShotEventReplication",
						[&promise](const Direction dir, const buuid& uuid) { promise.set_value({dir, uuid}); });

	// events->EmitEvent("Server_StartFrame");
	events->EmitEvent("ServerSend_Shot", name, direction, _uuid);
	events->EmitEvent("Server_EndFrame");

	const auto status = future.wait_for(std::chrono::milliseconds(1000));
	events->EmitEvent("NetCommandUpdate", 1.0);
	events->EmitEvent("PreTickUpdate", 1.0);
	// ASSERT_EQ(status, std::future_status::ready);
	// ASSERT_EQ(future._Is_ready(), true);

	const auto [dirReplicated, uuidReplicated] = future.get();

	EXPECT_EQ(direction, dirReplicated);
	EXPECT_EQ(_uuid, uuidReplicated);

	events->RemoveListener("ClientReceived_" + name + "Shot", "ShotEventReplication");
}

TEST_F(NetworkTest, HealthEventReplication)
{
	using buuid = boost::uuids::uuid;

	auto events = std::make_shared<EventSystem>();
	auto server = std::make_unique<network::commands::ServerHandler>(events);
	auto client = std::make_unique<network::commands::ClientHandler>(events);

	constexpr int healthOrigin{42};

	std::promise<int> promise{};
	auto future = promise.get_future();

	const auto name{std::string("TestTank")};
	const auto uuidStr = boost::uuids::to_string(_uuid);
	const auto nameWithUuid = name + uuidStr;

	events->AddListener("ClientReceived_" + nameWithUuid + "Health", "HealthEventReplication",
						[&promise](const int health) { promise.set_value(health); });

	//TODO: emit preUpdate to exec networkCommands, but before that extract it from game success to subscribe
	// events->EmitEvent("Server_StartFrame");
	events->EmitEvent("ServerSend_Health", name, healthOrigin, _uuid);
	events->EmitEvent("Server_EndFrame");

	const auto status = future.wait_for(std::chrono::milliseconds(1000));
	events->EmitEvent("NetCommandUpdate", 1.0);
	events->EmitEvent("PreTickUpdate", 1.0);
	// ASSERT_EQ(status, std::future_status::ready);
	// ASSERT_EQ(future._Is_ready(), true);

	const auto healthReplicated = future.get();
	EXPECT_EQ(healthOrigin, healthReplicated);

	events->RemoveListener("ClientReceived_" + nameWithUuid + "Health", "HealthEventReplication");
}

TEST_F(NetworkTest, DisposeEventReplication)
{
	using buuid = boost::uuids::uuid;

	auto events = std::make_shared<EventSystem>();
	auto server = std::make_unique<network::commands::ServerHandler>(events);
	auto client = std::make_unique<network::commands::ClientHandler>(events);

	std::promise<buuid> promise{};
	auto future = promise.get_future();

	const auto name{std::string("Bullet")};

	events->AddListener("ClientReceived_" + name + "Dispose", "DisposeEventReplication",
						[&promise](const buuid& uuid) { promise.set_value(uuid); });

	// events->EmitEvent("Server_StartFrame");
	events->EmitEvent("ServerSend_Dispose", _uuid);
	events->EmitEvent("Server_EndFrame");

	const auto status = future.wait_for(std::chrono::milliseconds(1000));
	events->EmitEvent("NetCommandUpdate", 1.0);
	events->EmitEvent("PreTickUpdate", 1.0);
	// ASSERT_EQ(status, std::future_status::ready);
	// ASSERT_EQ(future._Is_ready(), true);

	const auto uuidReplicated = future.get();
	EXPECT_EQ(_uuid, uuidReplicated);

	events->RemoveListener("ClientReceived_" + name + "Dispose", "DisposeEventReplication");
}

//TODO: cover all statistics items like this
TEST_F(NetworkTest, StatisticsEventReplication)
{
	auto events = std::make_shared<EventSystem>();
	auto server = std::make_unique<network::commands::ServerHandler>(events);
	auto client = std::make_unique<network::commands::ClientHandler>(events);

	std::promise<std::tuple<std::string, std::string, std::string>> promise{};
	auto future = promise.get_future();

	events->AddListener(
			"ClientReceived_Statistics", "StatisticsEventReplication",
			[&promise](const std::string& type, const std::string& author, const std::string& fraction)
			{
				promise.set_value({type, author, fraction});
			});

	// events->EmitEvent("Server_StartFrame");
	events->EmitEvent("ServerSend_Statistics", "BulletHit", "author", "fraction");
	events->EmitEvent("Server_EndFrame");

	const auto status = future.wait_for(std::chrono::milliseconds(1000));
	events->EmitEvent("NetCommandUpdate", 1.0);
	events->EmitEvent("PreTickUpdate", 1.0);
	// ASSERT_EQ(status, std::future_status::ready);
	// ASSERT_EQ(future._Is_ready(), true);

	const auto& [type, author, fraction] = future.get();
	EXPECT_EQ("BulletHit", type);
	EXPECT_EQ("author", author);
	EXPECT_EQ("fraction", fraction);

	events->RemoveListener("ClientReceived_Statistics", "StatisticsEventReplication");
}

//TODO: write retry 3 times logic if failure
TEST_F(NetworkTest, FortressChangeEventReplication)
{
	using buuid = boost::uuids::uuid;

	auto events = std::make_shared<EventSystem>();
	auto server = std::make_unique<network::commands::ServerHandler>(events);
	auto client = std::make_unique<network::commands::ClientHandler>(events);

	std::promise<std::pair<std::string, buuid>> promiseDied{};
	auto futureDied = promiseDied.get_future();

	std::promise<std::pair<std::string, buuid>> promiseToBrick{};
	auto futureToBrick = promiseToBrick.get_future();

	std::promise<std::pair<std::string, buuid>> promiseToSteel{};
	auto futureToSteel = promiseToSteel.get_future();

	events->AddListener(
			"ClientReceived_FortressChange", "FortressChangeEventReplication",
			[&promiseDied, &promiseToBrick, &promiseToSteel, this](const std::string& state, const buuid& uuid)
			{
				if (uuid == this->_uuid)
				{
					if (state == "Died")
					{
						promiseDied.set_value({state, uuid});
					}
					else if (state == "ToBrick")
					{
						promiseToBrick.set_value({state, uuid});
					}
					else if (state == "ToSteel")
					{
						promiseToSteel.set_value({state, uuid});
					}
				}
			});

	// events->EmitEvent("Server_StartFrame");
	events->EmitEvent("ServerSend_FortressChange", "Died", _uuid);
	events->EmitEvent("ServerSend_FortressChange", "ToBrick", _uuid);
	events->EmitEvent("ServerSend_FortressChange", "ToSteel", _uuid);
	events->EmitEvent("Server_EndFrame");

	auto statusDied = futureDied.wait_for(std::chrono::milliseconds(1000));
	events->EmitEvent("NetCommandUpdate", 1.0);
	events->EmitEvent("PreTickUpdate", 1.0);
	// ASSERT_EQ(statusDied, std::future_status::ready);
	// ASSERT_EQ(statusDied._Is_ready(), true);
	//TODO: refactor this, is ready not compile only in this test, for other its compile and work fine,
	//best solution remove timeout and write like asyncAwait of some results.
	
	const auto& [stateDied, uuidDied] = futureDied.get();
	EXPECT_EQ("Died", stateDied);
	EXPECT_EQ(_uuid, uuidDied);

	const auto statusToBrick = futureToBrick.wait_for(std::chrono::milliseconds(1000));
	ASSERT_EQ(statusToBrick, std::future_status::ready);
	const auto& [stateToBrick, uuidToBrick] = futureToBrick.get();
	EXPECT_EQ("ToBrick", stateToBrick);
	EXPECT_EQ(_uuid, uuidToBrick);

	const auto statusToSteel = futureToSteel.wait_for(std::chrono::milliseconds(1000));
	ASSERT_EQ(statusToSteel, std::future_status::ready);
	const auto& [stateToSteel, uuidToSteel] = futureToSteel.get();
	EXPECT_EQ("ToSteel", stateToSteel);
	EXPECT_EQ(_uuid, uuidToSteel);

	events->RemoveListener("ClientReceived_FortressChange", "FortressChangeEventReplication");
}

TEST_F(NetworkTest, BonusSpawnEventReplication)
{
	using buuid = boost::uuids::uuid;

	auto events = std::make_shared<EventSystem>();
	auto server = std::make_unique<network::commands::ServerHandler>(events);
	auto client = std::make_unique<network::commands::ClientHandler>(events);

	std::promise<std::tuple<FPoint, BonusType, buuid>> promise{};
	auto future = promise.get_future();

	events->AddListener(
			"ClientReceived_BonusSpawn", "BonusSpawnEventReplication",
			[&promise](const FPoint& pos, const BonusType& bonusType, const buuid& uuid)
			{
				promise.set_value({pos, bonusType, uuid});
			});

	// events->EmitEvent("Server_StartFrame");
	constexpr FPoint pos{.x = 42.f, .y = 42.f};
	constexpr auto type{BonusType::Timer};
	events->EmitEvent("ServerSend_BonusSpawn", pos, type, _uuid);
	events->EmitEvent("Server_EndFrame");

	const auto status = future.wait_for(std::chrono::milliseconds(1000));
	events->EmitEvent("NetCommandUpdate", 1.0);
	events->EmitEvent("PreTickUpdate", 1.0);
	// ASSERT_EQ(status, std::future_status::ready);
	// ASSERT_EQ(future._Is_ready(), true);

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
	auto server = std::make_unique<network::commands::ServerHandler>(events);
	auto client = std::make_unique<network::commands::ClientHandler>(events);

	std::promise<buuid> promise;
	auto future = promise.get_future();

	events->AddListener(
			"ClientReceived_BonusDeSpawn", "BonusDeSpawnEventReplication",
			[&promise](const buuid& uuid) { promise.set_value(uuid); });

	// events->EmitEvent("Server_StartFrame");
	events->EmitEvent("ServerSend_BonusDeSpawn", _uuid);
	events->EmitEvent("Server_EndFrame");

	const auto status = future.wait_for(std::chrono::milliseconds(1000));
	events->EmitEvent("NetCommandUpdate", 1.0);
	events->EmitEvent("PreTickUpdate", 1.0);
	// ASSERT_EQ(status, std::future_status::ready);
	// ASSERT_EQ(future._Is_ready(), true);

	const auto uuidReplicated = future.get();
	EXPECT_EQ(_uuid, uuidReplicated);

	events->RemoveListener("ClientReceived_BonusDeSpawn", "BonusDeSpawnEventReplication");
}

TEST_F(NetworkTest, BonusStatusEventReplication)
{
	auto events = std::make_shared<EventSystem>();
	auto server = std::make_unique<network::commands::ServerHandler>(events);
	auto client = std::make_unique<network::commands::ClientHandler>(events);

	const auto nameOrigin{std::string("Player1")};
	constexpr bool isActiveOrigin{true};

	std::promise<bool> promise;
	auto future = promise.get_future();

	events->AddListener(
			"ClientReceived_" + nameOrigin + "BonusHelmet_Pickup", "BonusStatusEventReplication",
			[&promise](const bool isEnable) { promise.set_value(isEnable); });

	// events->EmitEvent("Server_StartFrame");
	events->EmitEvent("ServerSend_BonusHelmet_Pickup", nameOrigin, isActiveOrigin);
	events->EmitEvent("Server_EndFrame");

	const auto status = future.wait_for(std::chrono::milliseconds(1000));
	events->EmitEvent("NetCommandUpdate", 1.0);
	events->EmitEvent("PreTickUpdate", 1.0);
	// ASSERT_EQ(status, std::future_status::ready);
	// ASSERT_EQ(future._Is_ready(), true);

	const auto isEnable = future.get();
	EXPECT_EQ(isActiveOrigin, isEnable);

	events->RemoveListener("ClientReceived_" + nameOrigin + "BonusHelmet_Pickup", "BonusStatusEventReplication");
}

TEST_F(NetworkTest, ObstacleSpawnEventReplication)
{
	using buuid = boost::uuids::uuid;

	auto events = std::make_shared<EventSystem>();
	auto server = std::make_unique<network::commands::ServerHandler>(events);
	auto client = std::make_unique<network::commands::ClientHandler>(events);

	constexpr auto obstacleType = ObstacleType::Brick;
	constexpr ObjRectangle rectOrigin{.x = 42.0f, .y = 43.0f, .w = 44.0f, .h = 45.0f};

	std::promise<std::tuple<ObjRectangle, ObstacleType, buuid>> promise{};
	auto future = promise.get_future();

	events->AddListener(
			"ClientReceived_ObstacleSpawn", "ObstacleSpawnEventReplication",
			[&promise](const ObjRectangle rect, const ObstacleType type, const buuid& uuid)
			{
				promise.set_value({rect, type, uuid});
			});

	// events->EmitEvent("Server_StartFrame");
	events->EmitEvent("ServerSend_ObstacleSpawn", rectOrigin, obstacleType, _uuid);
	events->EmitEvent("Server_EndFrame");

	const auto status = future.wait_for(std::chrono::milliseconds(1000));
	events->EmitEvent("NetCommandUpdate", 1.0);
	events->EmitEvent("PreTickUpdate", 1.0);
	// ASSERT_EQ(status, std::future_status::ready);
	// ASSERT_EQ(future._Is_ready(), true);

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
	auto server = std::make_unique<network::commands::ServerHandler>(events);
	auto client = std::make_unique<network::commands::ClientHandler>(events);

	constexpr auto obstacleType = ObstacleType::Brick;
	std::vector<ObjRectangle> bricksRect;
	std::vector<ObjRectangle> bricksRectReplicated;
	constexpr int itemsInMassiveTest = 10000;
	bricksRect.reserve(itemsInMassiveTest);
	bricksRectReplicated.reserve(itemsInMassiveTest);
	for (size_t i = 0u; i < itemsInMassiveTest; ++i)
	{
		const auto value = static_cast<float>(i);
		bricksRect.emplace_back(value, value + 1, value + 2, value + 3);
	}

	std::vector<std::promise<std::tuple<ObjRectangle, ObstacleType, buuid>>> promises(itemsInMassiveTest);

	std::mutex mtx;
	std::atomic<size_t> count{0};
	events->AddListener(
			"ClientReceived_ObstacleSpawn", "MassiveObstacleSpawnEventReplication",
			[&promises, &count, &mtx](const ObjRectangle rect, const ObstacleType type, const buuid& uuid)
			{
				std::scoped_lock lock(mtx);

				const auto current = count.fetch_add(1);
				if (current < promises.size())
				{
					promises[current].set_value({rect, type, uuid});
				}
			});

	// events->EmitEvent("Server_StartFrame");
	for (size_t i = 0u; i < itemsInMassiveTest; ++i)
	{
		events->EmitEvent("ServerSend_ObstacleSpawn", bricksRect[i], obstacleType, _uuid);
	}
	events->EmitEvent("Server_EndFrame");

	if (bricksRect.size() == bricksRectReplicated.size())
		for (size_t i = 0u; i < itemsInMassiveTest; ++i)
		{
			auto future = promises[i].get_future();
			const auto status = future.wait_for(std::chrono::milliseconds(10000));
			events->EmitEvent("NetCommandUpdate", 1.0);
			events->EmitEvent("PreTickUpdate", 1.0);
			// ASSERT_EQ(status, std::future_status::ready);
			// ASSERT_EQ(future._Is_ready(), true);

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
	auto server = std::make_unique<network::commands::ServerHandler>(events);
	auto client = std::make_unique<network::commands::ClientHandler>(events);

	std::vector<std::promise<std::pair<TankType, buuid>>> promises(6);

	size_t count = 0;
	events->AddListener(
			"ClientReceived_RespawnTank", "RespawnTankEventReplication",
			[&promises, &count](const TankType type, const buuid& uuid)
			{
				promises[count++].set_value({type, uuid});
			});

	constexpr std::array tankTypes{
			TankType::PLAYER1,
			TankType::PLAYER2,
			TankType::ENEMY1,
			TankType::ENEMY2,
			TankType::ENEMY3,
			TankType::ENEMY4
	};

	// events->EmitEvent("Server_StartFrame");
	for (const auto tankType: tankTypes)
	{
		events->EmitEvent("ServerSend_RespawnTank", tankType, _uuid);
	}
	events->EmitEvent("Server_EndFrame");

	for (size_t i = 0u; i < tankTypes.size(); ++i)
	{
		auto future = promises[i].get_future();
		const auto status = future.wait_for(std::chrono::milliseconds(1000));
		events->EmitEvent("NetCommandUpdate", 1.0);
		events->EmitEvent("PreTickUpdate", 1.0);
		// ASSERT_EQ(status, std::future_status::ready);
		// ASSERT_EQ(future._Is_ready(), true);

		const auto& [typeReplicated, uuid] = future.get();
		EXPECT_EQ(tankTypes[i], typeReplicated);
		EXPECT_EQ(_uuid, uuid);
	}

	events->RemoveListener("ClientReceived_RespawnTank", "RespawnTankEventReplication");
}

//TODO: write test for respawn resource change
//TODO: other bonus effect replication test after write this replication
// TEST_F(NetworkTest, bonusKind...EventReplication) {
//TODO: check animation create replication
