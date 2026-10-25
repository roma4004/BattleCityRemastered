#pragma once

#include "enums/MatchRules.h"
#include <array>
#include <cstdint>
#include <optional>
#include <string_view>

namespace network::discovery
{
//NOTE: raw bytes, not ser20 - the reply must be readable by a client speaking another protocol version
inline constexpr std::string_view kProbe{"BC?1"};
inline constexpr std::string_view kReplyTag{"BC!3"};

//NOTE: bump on any change to AnyCommand or the link header - the reply is the last place to refuse an old client
inline constexpr std::uint16_t kProtocolVersion{5};

struct Reply final
{
	std::uint16_t protocolVersion{};
	std::uint16_t gamePort{};
	std::uint8_t seats{};
	//NOTE: answered even at zero - silence means no server; a full match tells the client to wait, not to redial
	std::uint8_t freeSeats{};
	MatchRules rules{};
	//NOTE: random per server - found by the beacon and the catalog under two addresses, it is shown once
	std::uint32_t serverId{};
};

inline constexpr std::size_t kReplySize{kReplyTag.size() + 11u};

//NOTE: written byte by byte, little end first - a struct on the wire would carry this machine's padding
[[nodiscard]] inline std::array<char, kReplySize> Pack(const Reply& reply) noexcept
{
	std::array<char, kReplySize> bytes{};
	for (std::size_t i{}; i < kReplyTag.size(); ++i)
	{
		bytes[i] = kReplyTag[i];
	}

	bytes[4] = static_cast<char>(reply.protocolVersion & 0xFFu);
	bytes[5] = static_cast<char>(reply.protocolVersion >> 8u);
	bytes[6] = static_cast<char>(reply.gamePort & 0xFFu);
	bytes[7] = static_cast<char>(reply.gamePort >> 8u);
	bytes[8] = static_cast<char>(reply.seats);
	bytes[9] = static_cast<char>(reply.freeSeats);
	bytes[10] = static_cast<char>(reply.rules);
	for (std::size_t i{}; i < sizeof(reply.serverId); ++i)
	{
		bytes[11u + i] = static_cast<char>(reply.serverId >> (8u * i));
	}

	return bytes;
}

[[nodiscard]] inline std::optional<Reply> Parse(const std::string_view datagram) noexcept
{
	if (datagram.size() != kReplySize || !datagram.starts_with(kReplyTag))
	{
		return std::nullopt;
	}

	const auto byteAt = [datagram](const std::size_t index)
	{
		return static_cast<std::uint16_t>(static_cast<unsigned char>(datagram[index]));
	};

	const auto seats{static_cast<std::uint8_t>(byteAt(8))};
	const auto freeSeats{static_cast<std::uint8_t>(byteAt(9))};
	//NOTE: no server has more seats free than it has
	if (freeSeats > seats)
	{
		return std::nullopt;
	}

	std::uint32_t serverId{};
	for (std::size_t i{}; i < sizeof(serverId); ++i)
	{
		serverId |= std::uint32_t{byteAt(11u + i)} << (8u * i);
	}

	return Reply{.protocolVersion = static_cast<std::uint16_t>(byteAt(4) | byteAt(5) << 8u),
				 .gamePort = static_cast<std::uint16_t>(byteAt(6) | byteAt(7) << 8u),
				 .seats = seats,
				 .freeSeats = freeSeats,
				 .rules = static_cast<MatchRules>(byteAt(10)),
				 .serverId = serverId};
}
}//namespace network::discovery
