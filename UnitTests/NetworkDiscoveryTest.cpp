#include "network/Discovery.h"
#include "network/DiscoveryBeacon.h"
#include "network/DiscoveryProbe.h"
#include "network/DiscoveryScan.h"
#include "network/Endpoints.h"
#include "enums/MatchRules.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v6.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/system/error_code.hpp>
#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <optional>
#include <ranges>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

// beacon and probe over a real loopback socket - the io_context is pumped by hand instead of on a thread
class NetworkDiscoveryTest : public testing::Test
{
protected:
	void SetUp() override
	{
		boost::asio::ip::udp::socket socket{_ioContext};
		boost::system::error_code ec;
		std::ignore = socket.open(boost::asio::ip::udp::v4(), ec);
		std::ignore = socket.bind({boost::asio::ip::address_v4::loopback(), network::kDiscoveryPort}, ec);
		if (ec)
		{
			GTEST_SKIP() << "the discovery port is taken by something else on this machine";
		}
	}

	//NOTE: the probe is non-blocking - both sides are pumped until a reply comes or the patience runs out
	[[nodiscard]] std::optional<network::discovery::Reply> Ask(network::DiscoveryProbe& probe)
	{
		for (int attempt{}; attempt < 100; ++attempt)
		{
			if (const auto reply{probe.Poll()})
			{
				return reply;
			}

			_ioContext.poll();
			std::this_thread::sleep_for(std::chrono::milliseconds{5});
		}

		return std::nullopt;
	}

	boost::asio::io_context _ioContext{};
};

// the beacon answers with the game port, the rules and the free seats
TEST_F(NetworkDiscoveryTest, ABeaconAnswersWithTheGamePortItsRulesAndItsFreeSeats)
{
	constexpr std::uint16_t gamePort{54321u};
	const network::DiscoveryBeacon beacon{_ioContext, boost::asio::ip::address_v4::loopback(), gamePort,
										  std::uint8_t{4}, MatchRules::FreeForAll, [] { return std::uint8_t{1}; }};

	network::DiscoveryProbe probe{std::string{network::kDefaultHost}};

	const auto reply{Ask(probe)};

	ASSERT_TRUE(reply.has_value());
	EXPECT_EQ(reply->gamePort, gamePort);
	EXPECT_EQ(reply->protocolVersion, network::discovery::kProtocolVersion);
	EXPECT_EQ(reply->seats, 4u);
	EXPECT_EQ(reply->freeSeats, 1u);
	EXPECT_EQ(reply->rules, MatchRules::FreeForAll);
}

// a full server still answers: silence would read as "no server here" and the client would keep looking
TEST_F(NetworkDiscoveryTest, AFullServerStillAnswers)
{
	const network::DiscoveryBeacon beacon{_ioContext, boost::asio::ip::address_v4::loopback(), 12345u,
										  std::uint8_t{4}, MatchRules::Classic, [] { return std::uint8_t{}; }};

	network::DiscoveryProbe probe{std::string{network::kDefaultHost}};

	const auto reply{Ask(probe)};

	ASSERT_TRUE(reply.has_value());
	EXPECT_EQ(reply->freeSeats, 0u);
}

// the scan asks this machine by its address too - its own broadcast may never come back to it
TEST_F(NetworkDiscoveryTest, AScanFindsTheServerOnThisMachine)
{
	constexpr std::uint16_t gamePort{54321u};
	const network::DiscoveryBeacon beacon{_ioContext, boost::asio::ip::address_v4::loopback(), gamePort,
										  std::uint8_t{4}, MatchRules::FreeForAll, [] { return std::uint8_t{2}; }};

	network::DiscoveryScan scan{std::string{network::kDefaultHost}};
	const network::FoundServer expected{
			.address = network::ServerAddress{.host = network::kDefaultHost, .port = gamePort}, .seats = 4u,
			.freeSeats = 2u,
			.rules = MatchRules::FreeForAll};

	for (int attempt{}; attempt < 100 && !std::ranges::contains(scan.Servers(), expected); ++attempt)
	{
		_ioContext.poll();
		std::ignore = scan.Poll();
		std::this_thread::sleep_for(std::chrono::milliseconds{5});
	}

	EXPECT_TRUE(std::ranges::contains(scan.Servers(), expected));
}

// IPv6 has no broadcast: a server listening over it is found by the group on its link, at an address it listens on -
// on "::" the one that reaches furthest from that link, on one address that address
TEST_F(NetworkDiscoveryTest, AScanFindsAnIPv6ServerByTheGroupOnItsLink)
{
	const std::vector<network::IPv6Interface> links{network::IPv6Interfaces()};
	boost::asio::ip::udp::socket socket{_ioContext};
	boost::system::error_code ec;
	std::ignore = socket.open(boost::asio::ip::udp::v6(), ec);
	std::ignore = socket.bind({boost::asio::ip::address_v6::any(), network::kDiscoveryPort}, ec);
	if (links.empty() || ec)
	{
		GTEST_SKIP() << "no IPv6 link on this machine, or its discovery port is taken";
	}

	std::ignore = socket.close(ec);
	//NOTE: a server on "::" may hear the IPv4 broadcast too - those answers are no concern here
	const auto findOverIPv6 = [](const std::string& host)
	{
		//NOTE: one per beacon - a receive completed after the last poll dies with it, not in the next beacon's poll
		boost::asio::io_context ioContext{};
		constexpr std::uint16_t gamePort{54321u};
		const network::DiscoveryBeacon beacon{ioContext, boost::asio::ip::make_address(host), gamePort,
											  std::uint8_t{4}, MatchRules::FreeForAll, [] { return std::uint8_t{2}; }};
		network::DiscoveryScan scan{std::string{}};
		const auto found = [&scan]
		{
			const auto isOverIPv6 = [](const network::FoundServer& server)
			{
				return server.address.port == gamePort && server.address.host.contains(':');
			};

			return scan.Servers() | std::views::filter(isOverIPv6) | std::ranges::to<std::vector>();
		};

		//NOTE: past the first answer too - every link is probed at once, and the others come right behind it
		for (int attempt{}; attempt < 100 && (found().empty() || attempt < 10); ++attempt)
		{
			ioContext.poll();
			std::ignore = scan.Poll();
			std::this_thread::sleep_for(std::chrono::milliseconds{5});
		}

		return found();
	};
	const auto hostOf = [](const network::FoundServer& server) { return server.address.host; };

	const std::vector<network::FoundServer> onEveryAddress{findOverIPv6("::")};
	ASSERT_FALSE(onEveryAddress.empty()) << "no link carried the probe to the server";
	for (const std::string& host: onEveryAddress | std::views::transform(hostOf))
	{
		EXPECT_TRUE(std::ranges::contains(links, host, &network::IPv6Interface::furthest))
				<< host << " is not the furthest address of a link";
	}

	//NOTE: best a link's own address where the link reaches further - the answer must not trade it for the further
	//one; on a link the group was just heard over, as a tunnel's may carry none
	const auto isOwnOfAHeardFurtherLink = [&links, &onEveryAddress, &hostOf](const std::string& host)
	{
		if (!host.starts_with("fe80:"))
		{
			return false;
		}

		const auto link{std::ranges::find(links, boost::asio::ip::make_address(host).to_v6().scope_id(),
										  &network::IPv6Interface::index)};

		return link != links.end() && link->furthest != host
			   && std::ranges::contains(onEveryAddress, link->furthest, hostOf);
	};
	const auto hosts{network::OwnAddresses() | std::views::transform(&network::OwnAddress::host)};
	const auto linkLocal{std::ranges::find_if(hosts, isOwnOfAHeardFurtherLink)};
	const std::string own{linkLocal == hosts.end() ? onEveryAddress.front().address.host : *linkLocal};
	SCOPED_TRACE(own);
	const std::vector<network::FoundServer> onOwnAddress{findOverIPv6(own)};
	ASSERT_FALSE(onOwnAddress.empty()) << "no link carried the probe to the server";
	for (const std::string& host: onOwnAddress | std::views::transform(hostOf))
	{
		EXPECT_EQ(host, own) << "the answer named an address the server does not listen on";
	}
}
