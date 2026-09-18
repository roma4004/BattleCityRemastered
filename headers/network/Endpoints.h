#pragma once

#include <cstdint>
#include <string>

namespace network
{
inline constexpr auto kDefaultHost{"127.0.0.1"};
//NOTE: what bind reads as "any free one" - the port nobody asked for. A server takes it happily and says
//which one it got; a client cannot dial it and has to be told the number
inline constexpr std::uint16_t kAnyFreePort{};

//NOTE: where the dedicated server listens and a client dials; an unspecified host (0.0.0.0) listens on every
//interface and is dialled on loopback
struct ServerAddress final
{
	std::string host{kDefaultHost};
	std::uint16_t port{kAnyFreePort};
};
}//namespace network
