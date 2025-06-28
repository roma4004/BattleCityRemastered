#include "components/BulletPool.h"
#include "components/EventSystem.h"
#include "entities/obstacles/BrickWall.h"
#include "entities/obstacles/FortressWall.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/Direction.h"
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
	auto server = std::make_unique<ServerHandler>(events);
	auto client = std::make_unique<ClientHandler>(events);

	constexpr FPoint posOrigin{42.f, 42.f};
	constexpr Direction directionOrigin{UP};

	std::promise<std::tuple<FPoint, Direction, buuid>> replicationPromise;
	auto replicationFuture = replicationPromise.get_future();

	events->AddListener<const FPoint, const Direction, const buuid&>(
			"ClientReceived_TestTankPos", "PosEventReplication",
			[&replicationPromise](const FPoint newPos, const Direction dir, const buuid& uuid) mutable
			{
				replicationPromise.set_value({newPos, dir, uuid});
			});

	events->EmitEvent("Server_StartFrame");
	events->EmitEvent<const std::string&, const FPoint, const Direction, const buuid&>(
			"ServerSend_Pos", "TestTank", posOrigin, directionOrigin, _uuid);
	events->EmitEvent("Server_EndFrame");

	const auto status = replicationFuture.wait_for(std::chrono::milliseconds(1000));
	ASSERT_EQ(status, std::future_status::ready);

	const auto& [posReplicated, dirReplicated, uuidReplicated] = replicationFuture.get();

	EXPECT_EQ(posOrigin, posReplicated);
	EXPECT_EQ(directionOrigin, dirReplicated);
	EXPECT_EQ(_uuid, uuidReplicated);
}

TEST_F(NetworkTest, ShotEventReplication)
{
	using buuid = boost::uuids::uuid;

	auto events = std::make_shared<EventSystem>();
	auto server = std::make_unique<ServerHandler>(events);
	auto client = std::make_unique<ClientHandler>(events);

	constexpr Direction direction{UP};

	std::promise<std::pair<Direction, buuid>> replicationPromise;
	auto replicationFuture = replicationPromise.get_future();

	events->AddListener<const Direction, const buuid&>(
			"ClientReceived_TestTankShot", "ShotEventReplication",
			[&replicationPromise](
			const Direction dir, const buuid& uuid) mutable
			{
				replicationPromise.set_value({dir, uuid});
			});

	events->EmitEvent("Server_StartFrame");
	events->EmitEvent<const std::string&, const Direction, const buuid&>(
			"ServerSend_Shot", "TestTank", direction, _uuid);
	events->EmitEvent("Server_EndFrame");

	const auto status = replicationFuture.wait_for(std::chrono::milliseconds(1000));
	ASSERT_EQ(status, std::future_status::ready);

	const auto [dirReplicated, uuidReplicated] = replicationFuture.get();

	EXPECT_EQ(direction, dirReplicated);
	EXPECT_EQ(_uuid, uuidReplicated);
}

TEST_F(NetworkTest, HealthEventReplication)
{
	using buuid = boost::uuids::uuid;

	auto events = std::make_shared<EventSystem>();
	auto server = std::make_unique<ServerHandler>(events);
	auto client = std::make_unique<ClientHandler>(events);

	constexpr int healthOrigin{42};

	std::promise<int> replicationPromise;
	auto replicationFuture = replicationPromise.get_future();

	const auto name = std::string("TestTank");
	const auto uuidStr = boost::uuids::to_string(_uuid);
	const auto nameWithUuid = name + uuidStr;

	events->AddListener<const int>(
			"ClientReceived_" + nameWithUuid + "Health", "HealthEventReplication",
			[&replicationPromise](const int health) { replicationPromise.set_value(health); });

	events->EmitEvent("Server_StartFrame");
	events->EmitEvent<const std::string&, const int, const buuid&>("ServerSend_Health", name, healthOrigin, _uuid);
	events->EmitEvent("Server_EndFrame");

	const auto status = replicationFuture.wait_for(std::chrono::milliseconds(1000));
	ASSERT_EQ(status, std::future_status::ready);

	const auto healthReplicated = replicationFuture.get();
	EXPECT_EQ(healthOrigin, healthReplicated);
}

TEST_F(NetworkTest, DisposeEventReplication)
{
	using buuid = boost::uuids::uuid;

	auto events = std::make_shared<EventSystem>();
	auto server = std::make_unique<ServerHandler>(events);
	auto client = std::make_unique<ClientHandler>(events);

	std::promise<buuid> replicationPromise;
	auto replicationFuture = replicationPromise.get_future();

	const auto name = std::string("Bullet");

	events->AddListener<const buuid&>(
			"ClientReceived_" + name + "Dispose", "DisposeEventReplication",
			[&replicationPromise](const buuid& uuid) { replicationPromise.set_value(uuid); });

	events->EmitEvent("Server_StartFrame");
	events->EmitEvent<const buuid&>("ServerSend_Dispose", _uuid);
	events->EmitEvent("Server_EndFrame");

	const auto status = replicationFuture.wait_for(std::chrono::milliseconds(1000));
	ASSERT_EQ(status, std::future_status::ready);

	const auto uuidReplicated = replicationFuture.get();
	EXPECT_EQ(_uuid, uuidReplicated);
}

// TEST_F(NetworkTest, StatisticsEventReplication) {
// TEST_F(NetworkTest, FortressChangeEventReplication) {
// TEST_F(NetworkTest, BonusSpawnEventReplication) {
// TEST_F(NetworkTest, BonusDeSpawnEventReplication) {
// TEST_F(NetworkTest, RespawnTankDeSpawnEventReplication) {
// TEST_F(NetworkTest, ObstacleSpawnEventReplication) {

//TODO: other bonus effect replication test after write this replication
// TEST_F(NetworkTest, ...SpawnEventReplication) {
