#pragma once

#include "Discovery.h"
#include <algorithm>
#include <array>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/address_v6.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string_view>

namespace network::catalog
{
//NOTE: a server that misses two heartbeats is dropped
inline constexpr std::chrono::seconds kHeartbeatStep{4};
inline constexpr std::chrono::seconds kSilenceLimit{10};
//NOTE: three lists per address in three seconds - enough for REFRESH, too few to flood a spoofed address
inline constexpr std::chrono::seconds kAnswerWindow{3};
inline constexpr std::uint8_t kAnswerBurst{3u};

//NOTE: the beacon's reply, then a port-closed byte
inline constexpr std::size_t kHeartbeatSize{discovery::kReplySize + 1u};
//NOTE: a heartbeat, then its sender's address - IPv4 mapped into IPv6
inline constexpr std::size_t kListingSize{kHeartbeatSize + 16u};
//NOTE: after the listings, none too - a scan tells an empty list from a probe past the answer limit
inline constexpr std::string_view kListEnd{"BC.1"};

struct Heartbeat final
{
	//NOTE: zero seats is a goodbye
	discovery::Reply reply{};
	//NOTE: the router refused the port or the provider's NAT is in front - closed to the internet
	bool isPortClosed{};
};

struct Listing final
{
	Heartbeat heartbeat{};
	boost::asio::ip::address host{};
};

[[nodiscard]] inline std::array<char, kHeartbeatSize> Pack(const Heartbeat& heartbeat)
{
	std::array<char, kHeartbeatSize> bytes{};
	std::ranges::copy(discovery::Pack(heartbeat.reply), bytes.begin());
	bytes.back() = static_cast<char>(heartbeat.isPortClosed);

	return bytes;
}

[[nodiscard]] inline std::optional<Heartbeat> ParseHeartbeat(const std::string_view datagram)
{
	if (datagram.size() != kHeartbeatSize || static_cast<unsigned char>(datagram.back()) > 1u)
	{
		return std::nullopt;
	}

	const auto reply{discovery::Parse(datagram.substr(0, discovery::kReplySize))};
	if (!reply)
	{
		return std::nullopt;
	}

	return Heartbeat{.reply = *reply, .isPortClosed = datagram.back() != 0};
}

[[nodiscard]] inline std::array<char, kListingSize> Pack(const Listing& listing)
{
	std::array<char, kListingSize> bytes{};
	std::ranges::copy(Pack(listing.heartbeat), bytes.begin());

	const boost::asio::ip::address& host{listing.host};
	const auto ipv6{host.is_v4() ? boost::asio::ip::make_address_v6(boost::asio::ip::v4_mapped, host.to_v4())
								 : host.to_v6()};
	std::ranges::copy(ipv6.to_bytes(), bytes.begin() + kHeartbeatSize);

	return bytes;
}

//NOTE: told apart from a beacon's reply by the length alone
[[nodiscard]] inline std::optional<Listing> ParseListing(const std::string_view datagram)
{
	if (datagram.size() != kListingSize)
	{
		return std::nullopt;
	}

	const auto heartbeat{ParseHeartbeat(datagram.substr(0, kHeartbeatSize))};
	if (!heartbeat)
	{
		return std::nullopt;
	}

	boost::asio::ip::address_v6::bytes_type bytes{};
	std::ranges::copy(datagram.substr(kHeartbeatSize), bytes.begin());
	const boost::asio::ip::address_v6 ipv6{bytes};
	if (ipv6.is_v4_mapped())
	{
		return Listing{.heartbeat = *heartbeat,
					   .host = boost::asio::ip::make_address_v4(boost::asio::ip::v4_mapped, ipv6)};
	}

	return Listing{.heartbeat = *heartbeat, .host = ipv6};
}
}//namespace network::catalog
