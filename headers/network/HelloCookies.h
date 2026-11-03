#pragma once

#include <array>
#include <boost/asio/ip/udp.hpp>
#include <chrono>
#include <cstdint>

namespace network
{
using boost::asio::ip::udp;

//NOTE: a server's answer to a hello, a catalog's to a heartbeat - only who hears at the sender echoes it
class HelloCookies final
{
public:
	using Clock = std::chrono::steady_clock;

	HelloCookies();

	[[nodiscard]] std::uint64_t For(const udp::endpoint& sender, std::uint32_t connectionId, Clock::time_point now) const;

	//NOTE: this window's or the one before - a cookie handed out just before the window turned still counts
	[[nodiscard]] bool IsValid(std::uint64_t cookie, const udp::endpoint& sender, std::uint32_t connectionId,
							   Clock::time_point now) const;

private:
	[[nodiscard]] std::uint64_t Of(const udp::endpoint& sender, std::uint32_t connectionId, std::int64_t window) const;

	//NOTE: random per server - the cookie of one address says nothing of another's
	std::array<std::uint64_t, 2u> _key{};
};
}//namespace network
