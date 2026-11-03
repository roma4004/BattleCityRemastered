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
#include <span>
#include <string_view>

namespace network::catalog
{
//NOTE: a server that misses two heartbeats is dropped
inline constexpr std::chrono::seconds kHeartbeatStep{4};
inline constexpr std::chrono::seconds kSilenceLimit{10};
//NOTE: three lists per address in three seconds - enough for REFRESH, too few to flood a spoofed address
inline constexpr std::chrono::seconds kAnswerWindow{3};
inline constexpr std::uint8_t kAnswerBurst{3u};
//NOTE: a few servers on one machine - more game ports from one address, or one IPv6 /64, would only fill every answer
inline constexpr std::ptrdiff_t kServersPerAddress{8};
//NOTE: more than a list shows - and every probe is answered with a datagram per server
inline constexpr std::size_t kMaxServers{64u};

//NOTE: the beacon's reply, then a port-closed byte - what a scan is told of a server
inline constexpr std::size_t kStatusSize{discovery::kReplySize + 1u};
//NOTE: a token, a cookie or a nonce
inline constexpr std::size_t kWordSize{8u};
//NOTE: the status, the server's token, then the catalog's cookie
inline constexpr std::size_t kHeartbeatSize{kStatusSize + kWordSize + kWordSize};
//NOTE: the answer to a heartbeat without a valid cookie - its token, then the cookie
inline constexpr std::string_view kChallenge{"BC#1"};
inline constexpr std::size_t kChallengeSize{kChallenge.size() + kWordSize + kWordSize};
//NOTE: the beacon's probe, then a nonce every answer echoes
inline constexpr std::size_t kQuerySize{discovery::kProbe.size() + kWordSize};
//NOTE: the status, the server's address - IPv4 mapped into IPv6 - then the query's nonce
inline constexpr std::size_t kListingSize{kStatusSize + 16u + kWordSize};
//NOTE: after the listings, none too - a scan tells an empty list from a probe past the answer limit
inline constexpr std::string_view kListEnd{"BC.1"};
inline constexpr std::size_t kListEndSize{kListEnd.size() + kWordSize};

struct Heartbeat final
{
	//NOTE: zero seats is a goodbye
	discovery::Reply reply{};
	//NOTE: the router refused the port or the provider's NAT is in front - closed to the internet
	bool isPortClosed{};
	//NOTE: the server's own, never listed - address, port and id go to every scan, so only this says who is speaking
	std::uint64_t token{};
	//NOTE: the catalog's, echoed - a forged sender never hears it
	std::uint64_t cookie{};
};

struct Listing final
{
	Heartbeat heartbeat{};
	boost::asio::ip::address host{};
	//NOTE: the query's - a catalog may answer from another of its addresses
	std::uint64_t nonce{};
};

struct Challenge final
{
	std::uint64_t token{};
	std::uint64_t cookie{};
};

//NOTE: little end first, as the reply's fields
inline void PutWord(const std::span<char, kWordSize> to, const std::uint64_t word)
{
	for (std::size_t byte{}; byte < kWordSize; ++byte)
	{
		to[byte] = static_cast<char>(word >> (8u * byte));
	}
}

[[nodiscard]] inline std::uint64_t TakeWord(const std::string_view from)
{
	std::uint64_t word{};
	for (std::size_t byte{}; byte < kWordSize; ++byte)
	{
		word |= std::uint64_t{static_cast<unsigned char>(from[byte])} << (8u * byte);
	}

	return word;
}

//NOTE: a query or a list end - a tag, then a word
[[nodiscard]] inline std::optional<std::uint64_t> ParseTagged(const std::string_view datagram,
															  const std::string_view tag)
{
	if (datagram.size() != tag.size() + kWordSize || !datagram.starts_with(tag))
	{
		return std::nullopt;
	}

	return TakeWord(datagram.substr(tag.size()));
}

[[nodiscard]] inline std::array<char, kQuerySize> PackQuery(const std::uint64_t nonce)
{
	std::array<char, kQuerySize> bytes{};
	std::ranges::copy(discovery::kProbe, bytes.begin());
	PutWord(std::span{bytes}.last<kWordSize>(), nonce);

	return bytes;
}

[[nodiscard]] inline std::array<char, kListEndSize> PackListEnd(const std::uint64_t nonce)
{
	std::array<char, kListEndSize> bytes{};
	std::ranges::copy(kListEnd, bytes.begin());
	PutWord(std::span{bytes}.last<kWordSize>(), nonce);

	return bytes;
}

[[nodiscard]] inline std::array<char, kChallengeSize> Pack(const Challenge& challenge)
{
	std::array<char, kChallengeSize> bytes{};
	std::ranges::copy(kChallenge, bytes.begin());
	PutWord(std::span{bytes}.subspan<kChallenge.size(), kWordSize>(), challenge.token);
	PutWord(std::span{bytes}.last<kWordSize>(), challenge.cookie);

	return bytes;
}

[[nodiscard]] inline std::optional<Challenge> ParseChallenge(const std::string_view datagram)
{
	if (datagram.size() != kChallengeSize || !datagram.starts_with(kChallenge))
	{
		return std::nullopt;
	}

	return Challenge{.token = TakeWord(datagram.substr(kChallenge.size())),
					 .cookie = TakeWord(datagram.substr(kChallenge.size() + kWordSize))};
}

[[nodiscard]] inline std::array<char, kStatusSize> PackStatus(const Heartbeat& heartbeat)
{
	std::array<char, kStatusSize> bytes{};
	std::ranges::copy(discovery::Pack(heartbeat.reply), bytes.begin());
	bytes.back() = static_cast<char>(heartbeat.isPortClosed);

	return bytes;
}

//NOTE: the token left at zero - the status alone does not carry it
[[nodiscard]] inline std::optional<Heartbeat> ParseStatus(const std::string_view status)
{
	if (status.size() != kStatusSize || static_cast<unsigned char>(status.back()) > 1u)
	{
		return std::nullopt;
	}

	const auto reply{discovery::Parse(status.substr(0, discovery::kReplySize))};
	if (!reply)
	{
		return std::nullopt;
	}

	return Heartbeat{.reply = *reply, .isPortClosed = status.back() != 0};
}

[[nodiscard]] inline std::array<char, kHeartbeatSize> Pack(const Heartbeat& heartbeat)
{
	std::array<char, kHeartbeatSize> bytes{};
	std::ranges::copy(PackStatus(heartbeat), bytes.begin());
	PutWord(std::span{bytes}.subspan<kStatusSize, kWordSize>(), heartbeat.token);
	PutWord(std::span{bytes}.last<kWordSize>(), heartbeat.cookie);

	return bytes;
}

[[nodiscard]] inline std::optional<Heartbeat> ParseHeartbeat(const std::string_view datagram)
{
	if (datagram.size() != kHeartbeatSize)
	{
		return std::nullopt;
	}

	const auto status{ParseStatus(datagram.substr(0, kStatusSize))};
	if (!status)
	{
		return std::nullopt;
	}

	return Heartbeat{.reply = status->reply,
					 .isPortClosed = status->isPortClosed,
					 .token = TakeWord(datagram.substr(kStatusSize)),
					 .cookie = TakeWord(datagram.substr(kStatusSize + kWordSize))};
}

[[nodiscard]] inline std::array<char, kListingSize> Pack(const Listing& listing)
{
	std::array<char, kListingSize> bytes{};
	std::ranges::copy(PackStatus(listing.heartbeat), bytes.begin());

	const boost::asio::ip::address& host{listing.host};
	const auto ipv6{host.is_v4() ? boost::asio::ip::make_address_v6(boost::asio::ip::v4_mapped, host.to_v4())
								 : host.to_v6()};
	std::ranges::copy(ipv6.to_bytes(), bytes.begin() + kStatusSize);
	PutWord(std::span{bytes}.last<kWordSize>(), listing.nonce);

	return bytes;
}

//NOTE: told apart from a beacon's reply by the length alone
[[nodiscard]] inline std::optional<Listing> ParseListing(const std::string_view datagram)
{
	if (datagram.size() != kListingSize)
	{
		return std::nullopt;
	}

	const auto heartbeat{ParseStatus(datagram.substr(0, kStatusSize))};
	if (!heartbeat)
	{
		return std::nullopt;
	}

	boost::asio::ip::address_v6::bytes_type bytes{};
	std::ranges::copy(datagram.substr(kStatusSize, bytes.size()), bytes.begin());
	const boost::asio::ip::address_v6 ipv6{bytes};
	const std::uint64_t nonce{TakeWord(datagram.substr(kStatusSize + bytes.size()))};
	if (ipv6.is_v4_mapped())
	{
		return Listing{.heartbeat = *heartbeat,
					   .host = boost::asio::ip::make_address_v4(boost::asio::ip::v4_mapped, ipv6),
					   .nonce = nonce};
	}

	return Listing{.heartbeat = *heartbeat, .host = ipv6, .nonce = nonce};
}
}//namespace network::catalog
