#include "network/Endpoints.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/address_v6.hpp>
#include <boost/asio/ip/network_v4.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/system/error_code.hpp>
#include <algorithm>
#include <charconv>
#include <cstddef>
#include <cstdint>
#include <cstring>
#include <optional>
#include <ranges>
#include <string>
#include <string_view>
#include <system_error>
#include <tuple>
#include <utility>
#include <vector>
#ifdef _WIN32 //NOTE: Windows
#include <winsock2.h>
#include <ws2tcpip.h>
#include <iphlpapi.h>
#else //NOTE: Linux
#include <ifaddrs.h>
#include <net/if.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <bit>
#include <functional>
#include <map>
#include <memory>
#endif //NOTE: Windows / Linux

namespace
{
//NOTE: any address past the local network will do
constexpr auto kOutsideAddress{"8.8.8.8"};
constexpr std::uint16_t kOutsidePort{53};
constexpr auto kIPv6Loopback{"::1"};
//NOTE: a /31 or /32 network is two machines or one - it has no broadcast address
constexpr unsigned short kWidestBroadcastPrefix{30u};

//NOTE: an interface's address and the length of its network's prefix
using InterfaceAddress = std::pair<boost::asio::ip::address, unsigned short>;

//NOTE: none for an interface address of a kind that is no IP
std::optional<boost::asio::ip::address> AddressOf(const sockaddr* const raw)
{
	if (raw == nullptr || (raw->sa_family != AF_INET && raw->sa_family != AF_INET6))
	{
		return std::nullopt;
	}

	boost::asio::ip::udp::endpoint endpoint;
	const std::size_t length{raw->sa_family == AF_INET6 ? sizeof(sockaddr_in6) : sizeof(sockaddr_in)};
	std::memcpy(endpoint.data(), raw, length);

	return endpoint.address();
}

//NOTE: the addresses of every interface that is up, one list per interface
#ifdef _WIN32 //NOTE: Windows
std::vector<std::vector<InterfaceAddress>> InterfaceAddresses()
{
	constexpr ULONG flags{GAA_FLAG_SKIP_ANYCAST | GAA_FLAG_SKIP_MULTICAST | GAA_FLAG_SKIP_DNS_SERVER};
	//NOTE: the size Microsoft suggests starting with - the call says how much it needs when that falls short
	ULONG size{15000u};
	std::vector<std::byte> buffer(size);
	auto* adapters{reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data())};
	ULONG result{GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, adapters, &size)};
	if (result == ERROR_BUFFER_OVERFLOW)
	{
		buffer.resize(size);
		adapters = reinterpret_cast<IP_ADAPTER_ADDRESSES*>(buffer.data());
		result = GetAdaptersAddresses(AF_UNSPEC, flags, nullptr, adapters, &size);
	}

	std::vector<std::vector<InterfaceAddress>> interfaces;
	if (result != NO_ERROR)
	{
		return interfaces;
	}

	for (const IP_ADAPTER_ADDRESSES* adapter{adapters}; adapter != nullptr; adapter = adapter->Next)
	{
		if (adapter->OperStatus != IfOperStatusUp)
		{
			continue;
		}

		std::vector<InterfaceAddress>& addresses{interfaces.emplace_back()};
		for (const IP_ADAPTER_UNICAST_ADDRESS* unicast{adapter->FirstUnicastAddress}; unicast != nullptr;
			 unicast = unicast->Next)
		{
			//NOTE: a temporary IPv6 on its way out stays listed for days - a server put on it would soon go dark
			if (unicast->DadState != IpDadStatePreferred)
			{
				continue;
			}

			if (const std::optional<boost::asio::ip::address> address{AddressOf(unicast->Address.lpSockaddr)})
			{
				addresses.emplace_back(*address, unicast->OnLinkPrefixLength);
			}
		}
	}

	return interfaces;
}
#else //NOTE: Linux
//NOTE: getifaddrs hands over the network's mask, not its length
unsigned short PrefixOf(const sockaddr* const mask)
{
	const std::optional<boost::asio::ip::address> ones{AddressOf(mask)};
	if (!ones)
	{
		return 0u;
	}

	if (ones->is_v4())
	{
		return static_cast<unsigned short>(std::popcount(ones->to_v4().to_uint()));
	}

	const auto bits = [](const unsigned char byte) { return std::popcount(byte); };

	return static_cast<unsigned short>(
			std::ranges::fold_left(ones->to_v6().to_bytes() | std::views::transform(bits), 0, std::plus{}));
}

std::vector<std::vector<InterfaceAddress>> InterfaceAddresses()
{
	ifaddrs* list{nullptr};
	if (getifaddrs(&list) != 0)
	{
		return {};
	}

	const std::unique_ptr<ifaddrs, decltype(&freeifaddrs)> owner{list, &freeifaddrs};
	std::map<std::string, std::vector<InterfaceAddress>> byName;
	for (const ifaddrs* entry{list}; entry != nullptr; entry = entry->ifa_next)
	{
		if ((entry->ifa_flags & IFF_UP) == 0u)
		{
			continue;
		}

		if (const std::optional<boost::asio::ip::address> address{AddressOf(entry->ifa_addr)})
		{
			byName[entry->ifa_name].emplace_back(*address, PrefixOf(entry->ifa_netmask));
		}
	}

	return byName | std::views::values | std::ranges::to<std::vector>();
}
#endif //NOTE: Windows / Linux

//NOTE: the lower, the more machines can dial it - the internet, then the site (fc00::/7), then the link alone
int ReachOf(const boost::asio::ip::address_v6& address)
{
	if (address.is_link_local())
	{
		return 2;
	}

	return (address.to_bytes().front() & 0xFEu) == 0xFCu ? 1 : 0;
}

bool HasBroadcast(const InterfaceAddress& entry)
{
	return entry.first.is_v4() && !entry.first.is_loopback() && entry.second <= kWidestBroadcastPrefix;
}

std::string BroadcastOf(const InterfaceAddress& entry)
{
	return boost::asio::ip::network_v4{entry.first.to_v4(), entry.second}.broadcast().to_string();
}

//NOTE: no route out - a switch and fixed addresses: the first address of a local network, a link's own one last
std::string OfflineAddress()
{
	const auto isOwnIPv4 = [](const boost::asio::ip::address& address)
	{
		return address.is_v4() && !address.is_loopback();
	};
	const auto ipv4{InterfaceAddresses() | std::views::join | std::views::keys | std::views::filter(isOwnIPv4)
					| std::ranges::to<std::vector>()};
	//NOTE: 169.254/16 is what Windows gives itself when nobody else gives it an address
	const auto isLinkLocal = [](const boost::asio::ip::address& address)
	{
		const auto bytes{address.to_v4().to_bytes()};

		return bytes[0] == 169u && bytes[1] == 254u;
	};
	const auto lan{std::ranges::find_if_not(ipv4, isLinkLocal)};
	if (lan != ipv4.end())
	{
		return lan->to_string();
	}

	return ipv4.empty() ? std::string{network::kDefaultHost} : ipv4.front().to_string();
}
}//namespace

namespace network
{
//NOTE: connecting UDP sends nothing - the OS only picks the outgoing interface, whose address others reach
std::string LocalAddress()
{
	using boost::asio::ip::udp;

	boost::asio::io_context ioContext;
	udp::socket socket{ioContext};
	boost::system::error_code ec;
	std::ignore = socket.open(udp::v4(), ec);
	if (!ec)
	{
		std::ignore = socket.connect(udp::endpoint{boost::asio::ip::make_address(kOutsideAddress), kOutsidePort}, ec);
	}

	if (ec)
	{
		return OfflineAddress();
	}

	const udp::endpoint local{socket.local_endpoint(ec)};
	if (ec || local.address().is_unspecified())
	{
		return OfflineAddress();
	}

	return local.address().to_string();
}

//NOTE: from the interface LocalAddress goes out by - its IPv6 that reaches furthest
std::string LocalIPv6Address()
{
	const auto local{boost::asio::ip::make_address(LocalAddress())};
	const std::vector<std::vector<InterfaceAddress>> interfaces{InterfaceAddresses()};
	const auto card{std::ranges::find_if(interfaces, [&local](const std::vector<InterfaceAddress>& addresses)
	{
		return std::ranges::contains(addresses | std::views::keys, local);
	})};
	if (card == interfaces.end())
	{
		return kIPv6Loopback;
	}

	auto ipv6{*card | std::views::keys | std::views::filter(&boost::asio::ip::address::is_v6)
			  | std::views::transform(&boost::asio::ip::address::to_v6)};
	const auto furthest{std::ranges::min_element(ipv6, {}, ReachOf)};

	return furthest == ipv6.end() ? kIPv6Loopback : (*furthest).to_string();
}

bool IsThisMachine(const std::string_view host)
{
	boost::system::error_code ec;
	const auto address{boost::asio::ip::make_address(std::string{host}, ec)};
	if (ec)
	{
		return false;
	}

	return address.is_loopback() || address.is_unspecified()
		   || std::ranges::contains(InterfaceAddresses() | std::views::join | std::views::keys, address);
}

std::vector<std::string> LocalBroadcasts()
{
	std::vector<std::string> broadcasts{InterfaceAddresses() | std::views::join | std::views::filter(HasBroadcast)
										| std::views::transform(BroadcastOf) | std::ranges::to<std::vector>()};
	std::ranges::sort(broadcasts);
	const auto repeats{std::ranges::unique(broadcasts)};
	broadcasts.erase(repeats.begin(), repeats.end());

	return broadcasts;
}

std::optional<std::string> SubnetBroadcast(const std::string_view host)
{
	boost::system::error_code ec;
	const auto address{boost::asio::ip::make_address(std::string{host}, ec)};
	if (ec)
	{
		return std::nullopt;
	}

	const std::vector<std::vector<InterfaceAddress>> interfaces{InterfaceAddresses()};
	const auto entries{interfaces | std::views::join};
	const auto own{std::ranges::find_if(entries, [&address](const InterfaceAddress& entry)
	{
		return entry.first == address && HasBroadcast(entry);
	})};

	return own == std::ranges::end(entries) ? std::nullopt : std::optional{BroadcastOf(*own)};
}

std::optional<std::string> ParseHost(const std::string_view value)
{
	//NOTE: Windows make_address takes "1.2.3.4:4321" and drops the port; one colon is a port, IPv6 has two
	if (value.contains('[') || value.contains(']') || std::ranges::count(value, ':') == 1)
	{
		return std::nullopt;
	}

	boost::system::error_code ec;
	const auto parsed{boost::asio::ip::make_address(std::string{value}, ec)};
	if (ec)
	{
		return std::nullopt;
	}

	return parsed.to_string();
}

std::optional<std::uint16_t> ParsePort(const std::string_view value)
{
	if (value == "auto")
	{
		return std::uint16_t{};
	}

	unsigned number{};
	const auto* const last{value.data() + value.size()};
	const auto [ptr, error] = std::from_chars(value.data(), last, number);
	if (error != std::errc{} || ptr != last || number > 65535u)
	{
		return std::nullopt;
	}

	return static_cast<std::uint16_t>(number);
}

std::optional<ServerAddress> ParseServerAddress(const std::string_view value)
{
	std::string_view host{value};
	std::optional<std::string_view> port{};
	if (value.starts_with('['))
	{
		const auto closing{value.find(']')};
		if (closing == std::string_view::npos)
		{
			return std::nullopt;
		}

		host = value.substr(1, closing - 1);
		if (const std::string_view rest{value.substr(closing + 1)}; !rest.empty())
		{
			if (!rest.starts_with(':'))
			{
				return std::nullopt;
			}

			port = rest.substr(1);
		}
	}
	else if (std::ranges::count(value, ':') == 1)
	{
		const auto colon{value.find(':')};
		host = value.substr(0, colon);
		port = value.substr(colon + 1);
	}
	else if (const auto colon{value.rfind(':')}; colon != std::string_view::npos && !ParseHost(value))
	{
		host = value.substr(0, colon);
		port = value.substr(colon + 1);
	}

	const std::optional<std::string> parsedHost{ParseHost(host)};
	if (!parsedHost)
	{
		return std::nullopt;
	}

	if (!port)
	{
		return ServerAddress{.host = *parsedHost};
	}

	const std::optional<std::uint16_t> parsedPort{ParsePort(*port)};
	if (!parsedPort)
	{
		return std::nullopt;
	}

	return ServerAddress{.host = *parsedHost, .port = *parsedPort};
}
}//namespace network
