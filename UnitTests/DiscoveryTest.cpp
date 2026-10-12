#include "network/Discovery.h"
#include "network/DiscoveryBeacon.h"
#include "network/DiscoveryProbe.h"
#include "network/DiscoveryScan.h"
#include "network/Endpoints.h"
#include "enums/MatchRules.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <gtest/gtest.h>
#include <algorithm>
#include <array>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>

// not an answer: the probe itself, a reply a byte short, someone else's datagram, more free seats than seats
TEST(DiscoveryFormatTest, WhatIsNotAReplyIsRefused)
{
	const auto reply{network::discovery::Pack(network::discovery::Reply{})};
	const auto overfull{network::discovery::Pack(network::discovery::Reply{.seats = 2u, .freeSeats = 3u})};

	const std::array notReplies{network::discovery::kProbe,
								std::string_view{reply.data(), reply.size() - 1u},
								std::string_view{"hello there"},
								std::string_view{overfull.data(), overfull.size()}};
	for (const std::string_view datagram: notReplies)
	{
		SCOPED_TRACE(datagram.size());
		EXPECT_FALSE(network::discovery::Parse(datagram).has_value());
	}
}

// beacon and probe over a real loopback socket - the io_context is pumped by hand instead of on a thread
class DiscoveryBeaconTest : public ::testing::Test
{
protected:
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
TEST_F(DiscoveryBeaconTest, ABeaconAnswersWithTheGamePortItsRulesAndItsFreeSeats)
{
	constexpr std::uint16_t gamePort{54321u};
	const network::DiscoveryBeacon beacon{_ioContext, boost::asio::ip::address_v4::loopback(), gamePort,
										  std::uint8_t{4}, MatchRules::FreeForAll, [] { return std::uint8_t{1}; }};
	if (!beacon.IsListening())
	{
		GTEST_SKIP() << "the discovery port is taken by something else on this machine";
	}

	network::DiscoveryProbe probe{std::string{network::kDefaultHost}};

	const auto reply{Ask(probe)};

	ASSERT_TRUE(reply.has_value());
	EXPECT_EQ(reply->gamePort, gamePort);
	EXPECT_EQ(reply->protocolVersion, network::discovery::kProtocolVersion);
	EXPECT_EQ(reply->seats, 4u);
	EXPECT_EQ(reply->freeSeats, 1u);
	EXPECT_EQ(reply->rules, MatchRules::FreeForAll);
}

// silence is the answer when nobody is up - it is what keeps the client asking instead of dialling
TEST_F(DiscoveryBeaconTest, AProbeWithNoBeaconGetsNothing)
{
	//NOTE: a beacon up and down first on a context never pumped - if it cannot listen, a server runs here
	{
		boost::asio::io_context checkContext{};
		const network::DiscoveryBeacon portCheck{checkContext, boost::asio::ip::address_v4::loopback(), 1u,
												 std::uint8_t{4}, MatchRules::Classic, [] { return std::uint8_t{}; }};
		if (!portCheck.IsListening())
		{
			GTEST_SKIP() << "the discovery port is taken by something else on this machine";
		}
	}

	network::DiscoveryProbe probe{std::string{network::kDefaultHost}};

	EXPECT_FALSE(Ask(probe).has_value());
}

// a full server still answers: silence would read as "no server here" and the client would keep looking
TEST_F(DiscoveryBeaconTest, AFullServerStillAnswers)
{
	const network::DiscoveryBeacon beacon{_ioContext, boost::asio::ip::address_v4::loopback(), 12345u,
										  std::uint8_t{4}, MatchRules::Classic, [] { return std::uint8_t{}; }};
	if (!beacon.IsListening())
	{
		GTEST_SKIP() << "the discovery port is taken by something else on this machine";
	}

	network::DiscoveryProbe probe{std::string{network::kDefaultHost}};

	const auto reply{Ask(probe)};

	ASSERT_TRUE(reply.has_value());
	EXPECT_EQ(reply->freeSeats, 0u);
}

// the well-known port takes one server - the second says so rather than fighting for it
TEST_F(DiscoveryBeaconTest, ASecondBeaconDoesNotListen)
{
	const network::DiscoveryBeacon first{_ioContext, boost::asio::ip::address_v4::loopback(), 1u,
										 std::uint8_t{4}, MatchRules::Classic, [] { return std::uint8_t{2}; }};
	if (!first.IsListening())
	{
		GTEST_SKIP() << "the discovery port is taken by something else on this machine";
	}

	boost::asio::io_context otherContext{};
	const network::DiscoveryBeacon second{otherContext, boost::asio::ip::address_v4::loopback(), 2u,
										  std::uint8_t{4}, MatchRules::Classic, [] { return std::uint8_t{2}; }};

	EXPECT_FALSE(second.IsListening());
}

// the scan asks this machine by its address too - its own broadcast may never come back to it
TEST_F(DiscoveryBeaconTest, AScanFindsTheServerOnThisMachine)
{
	constexpr std::uint16_t gamePort{54321u};
	const network::DiscoveryBeacon beacon{_ioContext, boost::asio::ip::address_v4::loopback(), gamePort,
										  std::uint8_t{4}, MatchRules::FreeForAll, [] { return std::uint8_t{2}; }};
	if (!beacon.IsListening())
	{
		GTEST_SKIP() << "the discovery port is taken by something else on this machine";
	}

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
