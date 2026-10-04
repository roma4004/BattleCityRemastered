#pragma once

#include <array>
#include <cstddef>
#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <utility>

namespace network::stun
{
//NOTE: RFC 5389 - a binding request is a bare 20-byte header, and the answer says which address and port
//the request came from as the server saw it
inline constexpr std::size_t kHeaderSize{20u};
inline constexpr std::uint32_t kMagicCookie{0x2112A442u};
inline constexpr std::uint16_t kBindingRequest{0x0001u};
inline constexpr std::uint16_t kBindingSuccess{0x0101u};
inline constexpr std::uint16_t kMappedAddress{0x0001u};
inline constexpr std::uint16_t kXorMappedAddress{0x0020u};
inline constexpr std::uint8_t kFamilyIPv4{0x01u};

using Transaction = std::array<std::uint8_t, 12>;

struct Server final
{
	std::string_view host{};
	std::string_view service{};
};

//NOTE: three of them, so one down or blocked leaves two - the first answer wins
inline constexpr std::array kPublicServers{Server{.host = "stun.l.google.com", .service = "19302"},
										   Server{.host = "stun.cloudflare.com", .service = "3478"},
										   Server{.host = "stun1.l.google.com", .service = "19302"}};

[[nodiscard]] inline std::array<char, kHeaderSize> Request(const Transaction& transaction) noexcept
{
	std::array<char, kHeaderSize> bytes{};
	bytes[0] = static_cast<char>(kBindingRequest >> 8u);
	bytes[1] = static_cast<char>(kBindingRequest & 0xFFu);
	//NOTE: bytes 2 and 3, the length of what follows the header, stay zero - a request carries nothing
	for (std::size_t i{}; i < 4u; ++i)
	{
		bytes[4u + i] = static_cast<char>((kMagicCookie >> (24u - 8u * i)) & 0xFFu);
	}

	for (std::size_t i{}; i < transaction.size(); ++i)
	{
		bytes[8u + i] = static_cast<char>(transaction[i]);
	}

	return bytes;
}

//NOTE: the IPv4 out of the answer to our own request - XOR-MAPPED-ADDRESS first, as servers since RFC 5389 send
//it; the plain MAPPED-ADDRESS of older ones only when it is missing. Anything else read is none
[[nodiscard]] inline std::optional<std::string> MappedAddress(const std::string_view datagram,
															 const Transaction& transaction)
{
	const auto byteAt = [datagram](const std::size_t index) { return static_cast<std::uint8_t>(datagram[index]); };
	const auto wordAt = [&byteAt](const std::size_t index)
	{
		return static_cast<std::uint16_t>(byteAt(index) << 8u | byteAt(index + 1u));
	};
	const auto cookieByte = [](const std::size_t index)
	{
		return static_cast<std::uint8_t>((kMagicCookie >> (24u - 8u * index)) & 0xFFu);
	};

	std::optional<std::string> found{};
	if (datagram.size() < kHeaderSize || wordAt(0u) != kBindingSuccess)
	{
		return found;
	}

	for (std::size_t i{}; i < 4u; ++i)
	{
		if (byteAt(4u + i) != cookieByte(i))
		{
			return found;
		}
	}

	for (std::size_t i{}; i < transaction.size(); ++i)
	{
		if (byteAt(8u + i) != transaction[i])
		{
			return found;
		}
	}

	const std::size_t end{kHeaderSize + wordAt(2u)};
	if (end > datagram.size())
	{
		return found;
	}

	//NOTE: an attribute is its type, its length and a value padded to four bytes
	for (std::size_t at{kHeaderSize}; at + 4u <= end;)
	{
		const std::uint16_t type{wordAt(at)};
		const std::size_t length{wordAt(at + 2u)};
		const std::size_t value{at + 4u};
		if (value + length > end)
		{
			break;
		}

		const bool isXored{type == kXorMappedAddress};
		if ((isXored || type == kMappedAddress) && length >= 8u && byteAt(value + 1u) == kFamilyIPv4)
		{
			std::string text{};
			for (std::size_t i{}; i < 4u; ++i)
			{
				const std::uint8_t octet{static_cast<std::uint8_t>(byteAt(value + 4u + i)
																   ^ (isXored ? cookieByte(i) : 0u))};
				text += (i == 0u ? "" : ".") + std::to_string(octet);
			}

			found = std::move(text);
			if (isXored)
			{
				break;
			}
		}

		at = value + (length + 3u) / 4u * 4u;
	}

	return found;
}
}//namespace network::stun
