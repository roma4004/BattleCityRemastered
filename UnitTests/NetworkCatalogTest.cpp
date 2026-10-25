#include "components/EventSystem.h"
#include "components/MatchSettings.h"
#include "components/events/CoreLifecycleEvents.h"
#include "enums/MatchRules.h"
#include "enums/PlayerSlot.h"
#include "enums/PortForwarding.h"
#include "network/DiscoveryScan.h"
#include "network/Endpoints.h"
#include "network/ServerCatalog.h"
#include "network/ServerNode.h"
#include <boost/asio/io_context.hpp>
#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <memory>
#include <string>
#include <thread>
#include <tuple>

using namespace std::chrono_literals;

// real loopback sockets - the catalog pumped by hand, the server on its own thread
class NetworkCatalogTest : public ::testing::Test
{
protected:
	boost::asio::io_context _ioContext{};
	//NOTE: dual-stack as in catalog_main - the scan asks it over IPv4
	network::ServerCatalog _catalog{_ioContext, network::ServerAddress{.host = "::", .port = network::kAnyFreePort}};
	const network::ServerAddress _catalogAddress{.port = _catalog.BoundPort()};
	std::shared_ptr<EventSystem> _serverEvents{std::make_shared<EventSystem>()};
	std::unique_ptr<network::ServerNode> _server{std::make_unique<network::ServerNode>(
			network::ServerAddress{.port = network::kAnyFreePort}, _serverEvents,
			MatchSettings{.rules = MatchRules::FreeForAll, .seats = 3u}, _catalogAddress)};
	//NOTE: no own host - a loopback server hears no broadcast, only the catalog finds it
	network::DiscoveryScan _scan{std::string{}, {_catalogAddress}};
	const network::FoundServer _listed{
			.address = network::ServerAddress{.host = network::kDefaultHost, .port = _server->GetBoundPort()},
			.seats = 3u,
			.freeSeats = 3u,
			.rules = MatchRules::FreeForAll};

	//NOTE: pumps both sides until the server is listed or not, as asked, for two seconds at most
	[[nodiscard]] bool Await(network::DiscoveryScan& scan, const network::FoundServer& server, const bool isListed)
	{
		for (int attempt{}; attempt < 400 && std::ranges::contains(scan.Servers(), server) != isListed; ++attempt)
		{
			_ioContext.poll();
			std::ignore = scan.Poll();
			std::this_thread::sleep_for(5ms);
		}

		return std::ranges::contains(scan.Servers(), server) == isListed;
	}

	//NOTE: pumps both sides for the whole span - whatever arrives in it is read
	void Pump(network::DiscoveryScan& scan, const std::chrono::milliseconds span)
	{
		const auto until{std::chrono::steady_clock::now() + span};
		while (std::chrono::steady_clock::now() < until)
		{
			_ioContext.poll();
			std::ignore = scan.Poll();
			std::this_thread::sleep_for(5ms);
		}
	}
};

// a client finds a server only the catalog knows of
TEST_F(NetworkCatalogTest, AServerListedInTheCatalogIsFoundByAScan)
{
	EXPECT_TRUE(Await(_scan, _listed, true));
}

// the same catalog lists a server on IPv6
TEST_F(NetworkCatalogTest, AServerOnIPv6IsFoundThroughTheSameCatalog)
{
	if (!network::IsReachable("::1"))
	{
		GTEST_SKIP() << "no IPv6 loopback on this machine";
	}

	const network::ServerNode server{network::ServerAddress{.host = "::1", .port = network::kAnyFreePort},
									 std::make_shared<EventSystem>(), MatchSettings{},
									 network::ServerAddress{.host = "::1", .port = _catalog.BoundPort()}};
	//NOTE: asked over IPv6 - a machine without IPv4 still gets the list
	network::DiscoveryScan scan{std::string{}, {network::ServerAddress{.host = "::1", .port = _catalog.BoundPort()}}};
	const network::FoundServer listed{.address = network::ServerAddress{.host = "::1", .port = server.GetBoundPort()},
									  .seats = kDefaultSeats,
									  .freeSeats = kDefaultSeats};

	EXPECT_TRUE(Await(scan, listed, true));
}

// a stopped server says goodbye and leaves the list at once, not after its silence
TEST_F(NetworkCatalogTest, AStoppedServerLeavesTheListAtOnce)
{
	ASSERT_TRUE(Await(_scan, _listed, true));

	_server.reset();
	_scan.Refresh();

	EXPECT_TRUE(Await(_scan, _listed, false));
}

// a port the router refused is marked in the list - the players know to swap hosts
TEST_F(NetworkCatalogTest, AServerTheRouterRefusedIsListedWithItsPortClosed)
{
	ASSERT_TRUE(Await(_scan, _listed, true));
	network::FoundServer closed{_listed};
	closed.isPortClosed = true;

	_serverEvents->EmitEvent(PortForwardingChangedEvent{.state = PortForwarding::Refused});
	_scan.Refresh();

	EXPECT_TRUE(Await(_scan, closed, true));
}

// REFRESH pressed over and over keeps the servers from the internet - the catalog skips a probe past its limit
TEST_F(NetworkCatalogTest, AListRefreshedOverAndOverKeepsTheServersFromTheInternet)
{
	ASSERT_TRUE(Await(_scan, _listed, true));

	//NOTE: the first probe and two refreshes are answered, the third refresh is past the catalog's answer limit
	for (int refresh{}; refresh < 3; ++refresh)
	{
		_scan.Refresh();
		Pump(_scan, 50ms);
	}

	//NOTE: past the round the last refresh opened, which got no answer
	Pump(_scan, 1100ms);

	EXPECT_TRUE(std::ranges::contains(_scan.Servers(), _listed));
}

// a host on the client's own network is listed at the address its beacon answered from, not again at the catalog's
TEST_F(NetworkCatalogTest, AServerOnTheLocalNetworkIsNotListedAgainByTheCatalog)
{
	//NOTE: the fixture's server holds the beacon's port on loopback
	_server.reset();
	const network::ServerNode server{network::ServerAddress{.host = "0.0.0.0", .port = network::kAnyFreePort},
									 std::make_shared<EventSystem>(), MatchSettings{}, _catalogAddress};
	network::DiscoveryScan scan{network::LocalAddress(), {_catalogAddress}};
	const network::FoundServer local{
			.address = network::ServerAddress{.host = network::LocalAddress(), .port = server.GetBoundPort()},
			.seats = kDefaultSeats,
			.freeSeats = kDefaultSeats};

	ASSERT_TRUE(Await(scan, local, true));
	//NOTE: the catalog's listing of it comes from loopback - long enough for it to arrive too
	Pump(scan, 100ms);

	const network::ServerAddress listedByCatalog{.host = network::kDefaultHost, .port = server.GetBoundPort()};
	EXPECT_FALSE(std::ranges::contains(scan.Servers(), listedByCatalog, &network::FoundServer::address));
}
