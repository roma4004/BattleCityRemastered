#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <string_view>
#include <vector>

namespace network
{
inline constexpr auto kDefaultHost{"127.0.0.1"};
//NOTE: a server reports the port it got, a client cannot dial it
inline constexpr std::uint16_t kAnyFreePort{};

//NOTE: the fixed discovery port - a server answers here with its game port; a match never runs on it
inline constexpr std::uint16_t kDiscoveryPort{12340};

//NOTE: where the server listens and a client dials; 0.0.0.0 listens on every interface and is dialled on loopback
struct ServerAddress final
{
	std::string host{kDefaultHost};
	std::uint16_t port{kAnyFreePort};

	[[nodiscard]] bool operator==(const ServerAddress& rhs) const = default;
};

//NOTE: an address of this machine and the interface it is on, by the name the system shows
struct OwnAddress final
{
	std::string host{};
	std::string interfaceName{};
};

//NOTE: this machine's address another machine can dial - a local network's with no route out, loopback with none
[[nodiscard]] std::string LocalAddress();

//NOTE: the IPv6 of the same interface, the one most machines can dial - loopback when it has none
[[nodiscard]] std::string LocalIPv6Address();

//NOTE: loopback, every interface, or an address one of them has - a literal only, as in ParseHost
[[nodiscard]] bool IsThisMachine(std::string_view host);

//NOTE: every address of every interface that is up, loopback among them
[[nodiscard]] std::vector<OwnAddress> OwnAddresses();

//NOTE: the broadcast address of every IPv4 network this machine is in - 255.255.255.255 leaves by one interface
[[nodiscard]] std::vector<std::string> LocalBroadcasts();

//NOTE: the broadcast address of the network this machine's IPv4 is in - none for an address it does not have
[[nodiscard]] std::optional<std::string> SubnetBroadcast(std::string_view host);

//NOTE: a literal only - a name is not resolved
[[nodiscard]] std::optional<std::string> ParseHost(std::string_view value);

//NOTE: "auto" and "0" both ask bind for any free port
[[nodiscard]] std::optional<std::uint16_t> ParsePort(std::string_view value);

//NOTE: an IPv6 port only after brackets, or when the whole text is no address - "fe80::1:5000" has none
[[nodiscard]] std::optional<ServerAddress> ParseServerAddress(std::string_view value);
}//namespace network
