#include "network/Endpoints.h"
#include "gtest/gtest.h"
#include <optional>
#include <string>
#include <string_view>

TEST(EndpointsTest, AnAddressWithoutAPortTakesAnyFreeOne)
{
	const std::optional<network::ServerAddress> address{network::ParseServerAddress("192.168.1.5")};

	ASSERT_TRUE(address.has_value());
	EXPECT_EQ(address->host, "192.168.1.5");
	EXPECT_EQ(address->port, network::kAnyFreePort);
}

TEST(EndpointsTest, APortFollowsTheAddressAfterAColon)
{
	const std::optional<network::ServerAddress> address{network::ParseServerAddress("192.168.1.5:5000")};

	ASSERT_TRUE(address.has_value());
	EXPECT_EQ(address->host, "192.168.1.5");
	EXPECT_EQ(address->port, 5000u);
}

TEST(EndpointsTest, AnIPv6PortGoesAfterTheBrackets)
{
	const std::optional<network::ServerAddress> address{network::ParseServerAddress("[::1]:5000")};

	ASSERT_TRUE(address.has_value());
	EXPECT_EQ(address->host, "::1");
	EXPECT_EQ(address->port, 5000u);
}

TEST(EndpointsTest, AnIPv6PortIsWhatIsLeftPastAFullAddress)
{
	const std::optional<network::ServerAddress> address{network::ParseServerAddress("2001:db8:0:0:0:0:0:1:5000")};

	ASSERT_TRUE(address.has_value());
	EXPECT_EQ(address->host, "2001:db8::1");
	EXPECT_EQ(address->port, 5000u);
}

// the interface a link-local address goes out by is this machine's own, and the address is no use without it
TEST(EndpointsTest, AnIPv6ScopeIdStaysWithTheAddress)
{
	const std::optional<network::ServerAddress> address{network::ParseServerAddress("[fe80::1%12]:5000")};

	ASSERT_TRUE(address.has_value());
	EXPECT_EQ(address->host, "fe80::1%12");
	EXPECT_EQ(address->port, 5000u);
}

// "::" lets the last group belong to the address
TEST(EndpointsTest, AnIPv6ThatReadsWholeHasNoPort)
{
	const std::optional<network::ServerAddress> address{network::ParseServerAddress("fe80::1:5000")};

	ASSERT_TRUE(address.has_value());
	EXPECT_EQ(address->host, "fe80::1:5000");
	EXPECT_EQ(address->port, network::kAnyFreePort);
}

TEST(EndpointsTest, LoopbackEveryInterfaceAndTheNetworkAddressAreThisMachine)
{
	for (const std::string& host: {std::string{"127.0.0.1"}, std::string{"::1"}, std::string{"0.0.0.0"},
								   network::LocalAddress()})
	{
		EXPECT_TRUE(network::IsThisMachine(host)) << host;
	}
}

// 192.0.2.0/24 is kept for documentation, so no machine has it
TEST(EndpointsTest, AnotherAddressIsNotThisMachine)
{
	EXPECT_FALSE(network::IsThisMachine("192.0.2.1"));
}

TEST(EndpointsTest, WhatIsNoAddressIsRefused)
{
	for (const std::string_view text: {"", "localhost", "192.168.1.5:", "192.168.1.5:70000", "192.168.1.5:port",
									   "[::1", "[::1]5000", "[::1]:"})
	{
		EXPECT_FALSE(network::ParseServerAddress(text).has_value()) << text;
	}
}
