#include "network/HelloCookies.h"
#include <array>
#include <bit>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <random>
#include <span>
#include <vector>

namespace network
{
namespace
{
//NOTE: SipHash-2-4 - keyed, and a cookie seen does not lead back to the key the way a plain mix would
[[nodiscard]] std::uint64_t SipHash(const std::array<std::uint64_t, 2u>& key, const std::span<const std::uint8_t> bytes)
{
	std::uint64_t v0{0x736f6d6570736575u ^ key[0]};
	std::uint64_t v1{0x646f72616e646f6du ^ key[1]};
	std::uint64_t v2{0x6c7967656e657261u ^ key[0]};
	std::uint64_t v3{0x7465646279746573u ^ key[1]};
	const auto round = [&v0, &v1, &v2, &v3]
	{
		v0 += v1;
		v1 = std::rotl(v1, 13) ^ v0;
		v0 = std::rotl(v0, 32);
		v2 += v3;
		v3 = std::rotl(v3, 16) ^ v2;
		v0 += v3;
		v3 = std::rotl(v3, 21) ^ v0;
		v2 += v1;
		v1 = std::rotl(v1, 17) ^ v2;
		v2 = std::rotl(v2, 32);
	};
	const auto compress = [&v0, &v3, &round](const std::uint64_t word)
	{
		v3 ^= word;
		round();
		round();
		v0 ^= word;
	};

	const std::size_t whole{bytes.size() / 8u * 8u};
	for (std::size_t at{}; at < whole; at += 8u)
	{
		std::uint64_t word{};
		for (std::size_t byte{}; byte < 8u; ++byte)
		{
			word |= std::uint64_t{bytes[at + byte]} << (8u * byte);
		}
		compress(word);
	}

	std::uint64_t last{std::uint64_t{bytes.size() & 0xFFu} << 56u};
	for (std::size_t byte{}; byte < bytes.size() - whole; ++byte)
	{
		last |= std::uint64_t{bytes[whole + byte]} << (8u * byte);
	}
	compress(last);

	v2 ^= 0xFFu;
	for (int finish{}; finish < 4; ++finish)
	{
		round();
	}

	return v0 ^ v1 ^ v2 ^ v3;
}

[[nodiscard]] std::int64_t WindowOf(const HelloCookies::Clock::time_point now)
{
	constexpr std::chrono::seconds window{30};

	return std::chrono::duration_cast<std::chrono::seconds>(now.time_since_epoch()) / window;
}
}//namespace

HelloCookies::HelloCookies()
{
	std::random_device device{};
	std::uniform_int_distribution<std::uint64_t> any{};
	_key = {any(device), any(device)};
}

std::uint64_t HelloCookies::For(const udp::endpoint& sender, const std::uint32_t connectionId,
								const Clock::time_point now) const
{
	return Of(sender, connectionId, WindowOf(now));
}

bool HelloCookies::IsValid(const std::uint64_t cookie, const udp::endpoint& sender, const std::uint32_t connectionId,
						   const Clock::time_point now) const
{
	const std::int64_t window{WindowOf(now)};

	return cookie == Of(sender, connectionId, window) || cookie == Of(sender, connectionId, window - 1);
}

std::uint64_t HelloCookies::Of(const udp::endpoint& sender, const std::uint32_t connectionId,
							   const std::int64_t window) const
{
	std::vector<std::uint8_t> bytes{};
	if (sender.address().is_v4())
	{
		const auto address{sender.address().to_v4().to_bytes()};
		bytes.assign(address.begin(), address.end());
	}
	else
	{
		const auto address{sender.address().to_v6().to_bytes()};
		bytes.assign(address.begin(), address.end());
	}

	const auto append = [&bytes](const std::uint64_t value, const std::size_t size)
	{
		for (std::size_t byte{}; byte < size; ++byte)
		{
			bytes.push_back(static_cast<std::uint8_t>(value >> (8u * byte)));
		}
	};
	append(sender.port(), 2u);
	append(connectionId, 4u);
	append(static_cast<std::uint64_t>(window), 8u);

	return SipHash(_key, bytes);
}
}//namespace network
