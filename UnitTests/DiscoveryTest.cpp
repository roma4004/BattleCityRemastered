#include "network/Discovery.h"
#include "network/DiscoveryBeacon.h"
#include "network/DiscoveryProbe.h"
#include "network/DiscoveryScan.h"
#include "network/Endpoints.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <gtest/gtest.h>
#include <algorithm>
#include <chrono>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <tuple>

// the reply is raw bytes rather than a command, so the pack/parse pair is the whole contract
TEST(DiscoveryFormatTest, APackedReplyParsesBackToItself)
{
	constexpr network::discovery::Reply reply{.protocolVersion = 7u, .gamePort = 50000u, .freeSeats = 2u};

	const auto bytes{network::discovery::Pack(reply)};
	const auto parsed{network::discovery::Parse(std::string_view{bytes.data(), bytes.size()})};

	ASSERT_TRUE(parsed.has_value());
	EXPECT_EQ(parsed->protocolVersion, reply.protocolVersion);
	EXPECT_EQ(parsed->gamePort, reply.gamePort);
	EXPECT_EQ(parsed->freeSeats, reply.freeSeats);
}

// a port above 32767 is where a sign or a narrowed byte would show up
TEST(DiscoveryFormatTest, AHighPortSurvivesTheRoundTrip)
{
	constexpr network::discovery::Reply reply{.protocolVersion = 1u, .gamePort = 65535u, .freeSeats = 0u};

	const auto bytes{network::discovery::Pack(reply)};
	const auto parsed{network::discovery::Parse(std::string_view{bytes.data(), bytes.size()})};

	ASSERT_TRUE(parsed.has_value());
	EXPECT_EQ(parsed->gamePort, 65535u);
}

// the probe's own datagram must never read as an answer - both travel on the same well-known port
TEST(DiscoveryFormatTest, AProbeIsNotAReply)
{
	EXPECT_FALSE(network::discovery::Parse(network::discovery::kProbe).has_value());
}

// a reply one byte short is not read as a shorter one
TEST(DiscoveryFormatTest, ATruncatedReplyIsRefused)
{
	const auto bytes{network::discovery::Pack(network::discovery::Reply{})};

	EXPECT_FALSE(network::discovery::Parse(std::string_view{bytes.data(), bytes.size() - 1u}).has_value());
}

// and neither is a datagram that was never ours
TEST(DiscoveryFormatTest, SomeoneElsesDatagramIsRefused)
{
	EXPECT_FALSE(network::discovery::Parse("hello there").has_value());
}

// the beacon and the probe talk over a real loopback socket - the point is the two halves meeting,
// so the io_context is pumped by hand instead of running a thread for it
class DiscoveryBeaconTest : public ::testing::Test
{
protected:
	//NOTE: the probe is non-blocking, so an answer needs the beacon's handler to have run first - the
	//poll below pumps both sides until one of them produces a reply or the patience runs out
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

// a probe asking the well-known port is answered with the port the match runs on and the seats still free
TEST_F(DiscoveryBeaconTest, ABeaconAnswersWithTheGamePortAndItsFreeSeats)
{
	constexpr std::uint16_t gamePort{54321u};
	network::DiscoveryBeacon beacon{_ioContext, boost::asio::ip::address_v4::loopback(), gamePort,
									[] { return std::uint8_t{1}; }};
	if (!beacon.IsListening())
	{
		GTEST_SKIP() << "the discovery port is taken by something else on this machine";
	}

	network::DiscoveryProbe probe{std::string{network::kDefaultHost}};

	const auto reply{Ask(probe)};

	ASSERT_TRUE(reply.has_value());
	EXPECT_EQ(reply->gamePort, gamePort);
	EXPECT_EQ(reply->protocolVersion, network::discovery::kProtocolVersion);
	EXPECT_EQ(reply->freeSeats, 1u);
}

// silence is the answer when nobody is up - it is what keeps the client asking instead of dialling
TEST_F(DiscoveryBeaconTest, AProbeWithNoBeaconGetsNothing)
{
	network::DiscoveryProbe probe{std::string{network::kDefaultHost}};

	EXPECT_FALSE(probe.Poll().has_value());
}

// a full server still answers: silence would read as "no server here" and the client would keep looking
TEST_F(DiscoveryBeaconTest, AFullServerStillAnswers)
{
	network::DiscoveryBeacon beacon{_ioContext, boost::asio::ip::address_v4::loopback(), 12345u,
									[] { return std::uint8_t{}; }};
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
										 [] { return std::uint8_t{2}; }};
	if (!first.IsListening())
	{
		GTEST_SKIP() << "the discovery port is taken by something else on this machine";
	}

	boost::asio::io_context otherContext{};
	const network::DiscoveryBeacon second{otherContext, boost::asio::ip::address_v4::loopback(), 2u,
										  [] { return std::uint8_t{2}; }};

	EXPECT_FALSE(second.IsListening());
}

// the scan asks this machine by its address too - its own broadcast may never come back to it
TEST_F(DiscoveryBeaconTest, AScanFindsTheServerOnThisMachine)
{
	constexpr std::uint16_t gamePort{54321u};
	network::DiscoveryBeacon beacon{_ioContext, boost::asio::ip::address_v4::loopback(), gamePort,
									[] { return std::uint8_t{2}; }};
	if (!beacon.IsListening())
	{
		GTEST_SKIP() << "the discovery port is taken by something else on this machine";
	}

	network::DiscoveryScan scan{std::string{network::kDefaultHost}};
	const network::FoundServer expected{.host = network::kDefaultHost, .gamePort = gamePort, .freeSeats = 2u};

	for (int attempt{}; attempt < 100 && !std::ranges::contains(scan.Servers(), expected); ++attempt)
	{
		_ioContext.poll();
		std::ignore = scan.Poll();
		std::this_thread::sleep_for(std::chrono::milliseconds{5});
	}

	EXPECT_TRUE(std::ranges::contains(scan.Servers(), expected));
}
