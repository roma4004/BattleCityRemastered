#pragma once

#include "Catalog.h"
#include "Endpoints.h"
#include "enums/MatchRules.h"
#include <array>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/udp.hpp>
#include <chrono>
#include <cstdint>
#include <map>
#include <optional>
#include <string>
#include <vector>

namespace network
{
using boost::asio::ip::udp;

//NOTE: an address a server answered at, and what it said there - a stranger under its id speaks for itself
struct FoundAddress final
{
	//NOTE: the port is the game's, not the one the beacon answered from
	ServerAddress address{};
	std::uint8_t seats{};
	std::uint8_t freeSeats{};
	MatchRules rules{};
	//NOTE: the catalog's word only - an address its beacon answers at is reached without the router
	bool isPortClosed{};

	[[nodiscard]] bool IsFull() const noexcept { return freeSeats == 0u; }
	[[nodiscard]] bool operator==(const FoundAddress& rhs) const = default;
};

struct FoundServer final
{
	//NOTE: never empty - this machine's own, the local network's, the catalogs', loopback last; IPv4 first in each
	std::vector<FoundAddress> addresses{};

	[[nodiscard]] bool operator==(const FoundServer& rhs) const = default;
};

//NOTE: broadcast and the IPv6 group into every own network, its own address - a broadcast may not come back - catalogs
class DiscoveryScan final
{
public:
	//NOTE: a catalog of each protocol - it answers for the servers from the internet, of both protocols alike
	explicit DiscoveryScan(const std::string& ownHost, const std::vector<ServerAddress>& catalogs = {});

	DiscoveryScan(const DiscoveryScan&) = delete;
	DiscoveryScan& operator=(const DiscoveryScan&) = delete;
	DiscoveryScan(DiscoveryScan&&) = delete;
	DiscoveryScan& operator=(DiscoveryScan&&) = delete;

	//NOTE: a server that does not answer the new round drops off the list
	void Refresh();

	//NOTE: reads the answers and starts the next round when due; true when the list changed
	[[nodiscard]] bool Poll();

	//NOTE: a round is open while answers may still be on their way
	[[nodiscard]] bool IsSearching() const { return _isRoundOpen; }

	[[nodiscard]] std::vector<FoundServer> Servers() const;

private:
	using Clock = std::chrono::steady_clock;

	//NOTE: its answers echo the nonce, from whichever of its addresses they come
	struct AskedCatalog final
	{
		udp::endpoint endpoint{};
		std::uint64_t nonce{};
	};

	struct Heard final
	{
		FoundAddress found{};
		bool isAnswering{};
		//NOTE: the nonce of the catalog that listed it last - none when its beacon answered last
		std::optional<std::uint64_t> catalog{};
		Clock::time_point heardAt{};
		//NOTE: when its beacon answered last - it is the local network's only while it does
		std::optional<Clock::time_point> localHeardAt{};
	};

	//NOTE: a server, by its id - the beacon over each protocol and link, and every catalog, may tell of it
	struct Entry final
	{
		std::uint32_t serverId{};
		//NOTE: in the order FoundServer has them
		std::vector<Heard> addresses{};
	};

	//NOTE: true when the list changed
	[[nodiscard]] bool Read(udp::socket& socket);

	boost::asio::io_context _ioContext{};
	udp::socket _socket;
	//NOTE: the beacons' group and the catalogs - IPv6 has no broadcast
	udp::socket _socketIPv6;
	std::vector<udp::endpoint> _targets{};
	//NOTE: the addresses other machines dial this one at - a server here answers at all its networks'
	std::vector<std::string> _ownHosts{};
	//NOTE: the only ones whose listings and list ends count
	std::vector<AskedCatalog> _catalogs{};
	//NOTE: a catalog's listing is the longest datagram it reads; one byte more, or a longer one is cut to a listing
	std::array<char, catalog::kListingSize + 1u> _receiveBuffer{};
	std::vector<Entry> _entries{};
	//NOTE: the catalogs that ended their list this round - one past its answer limit stays silent
	std::vector<std::uint64_t> _answeredCatalogs{};
	//NOTE: whether a listed address has a route here - asked once, IsReachable opens a socket of its own
	std::map<boost::asio::ip::address, bool> _routes{};
	Clock::time_point _roundStart{};
	Clock::time_point _nextRound{};
	bool _isRoundOpen{};
};
}//namespace network
