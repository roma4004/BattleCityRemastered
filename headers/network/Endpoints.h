#pragma once

#include <cstdint>
#include <string>

namespace network
{
inline constexpr auto kDefaultHost{"127.0.0.1"};
//NOTE: what bind reads as "any free one" - the port nobody asked for. A server takes it happily and says
//which one it got; a client cannot dial it and has to be told the number
inline constexpr std::uint16_t kAnyFreePort{};

//NOTE: the one number nobody chooses - a server answers here with the game port it actually got, so a
//client that was given an address alone has somewhere to ask. Sits next to the 1234 the game used to
//default to, and is never what the match itself runs on
inline constexpr std::uint16_t kDiscoveryPort{12340};

//NOTE: where the dedicated server listens and a client dials; an unspecified host (0.0.0.0) listens on every
//interface and is dialled on loopback
struct ServerAddress final
{
	std::string host{kDefaultHost};
	std::uint16_t port{kAnyFreePort};
};
}//namespace network
