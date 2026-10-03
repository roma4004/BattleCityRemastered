#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

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

	[[nodiscard]] bool operator==(const ServerAddress& rhs) const = default;
};

//NOTE: this machine's address another machine can dial - a local network's with no route out, loopback with none
[[nodiscard]] std::string LocalAddress();

//NOTE: the IPv6 of the same interface, the one most machines can dial - loopback when it has none
[[nodiscard]] std::string LocalIPv6Address();

//NOTE: loopback, every interface, or an address one of them has - a literal only, as in ParseHost
[[nodiscard]] bool IsThisMachine(std::string_view host);

//NOTE: the broadcast address of every IPv4 network this machine is in - 255.255.255.255 leaves by one interface
[[nodiscard]] std::vector<std::string> LocalBroadcasts();

//NOTE: the broadcast address of the network this machine's IPv4 is in - none for an address it does not have
[[nodiscard]] std::optional<std::string> SubnetBroadcast(std::string_view host);

//NOTE: a literal only - a name is not resolved
[[nodiscard]] std::optional<std::string> ParseHost(std::string_view value);

//NOTE: "auto" and "0" both ask bind for any free port
[[nodiscard]] std::optional<std::uint16_t> ParsePort(std::string_view value);

//NOTE: IPv6 has a port in brackets, or when it is no address whole - "fe80::1:5000" has none
[[nodiscard]] std::optional<ServerAddress> ParseServerAddress(std::string_view value);
}//namespace network
