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

struct FoundServer final
{
	//NOTE: the port is the game's, not the one the beacon answered from
	ServerAddress address{};
	std::uint8_t seats{};
	std::uint8_t freeSeats{};
	MatchRules rules{};
	//NOTE: the catalog's word only - a server of the local network is reached without the router
	bool isPortClosed{};

	[[nodiscard]] bool IsFull() const noexcept { return freeSeats == 0u; }
	[[nodiscard]] bool operator==(const FoundServer& rhs) const = default;
};

//NOTE: broadcast into every own network, at its own address - a broadcast may not come back - and at the catalogs
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

	struct Entry final
	{
		FoundServer server{};
		std::uint32_t serverId{};
		bool isAnswering{};
		bool isLocal{};
		//NOTE: the catalog that listed it last - none when a beacon answered last
		std::optional<udp::endpoint> catalog{};
		Clock::time_point heardAt{};
	};

	//NOTE: what arrived on the socket; true when the list changed
	[[nodiscard]] bool Read(udp::socket& socket);

	boost::asio::io_context _ioContext{};
	udp::socket _socket;
	//NOTE: the catalog alone is asked over IPv6 - it has no broadcast
	udp::socket _socketIPv6;
	std::vector<udp::endpoint> _targets{};
	//NOTE: a catalog's listing is the longest datagram it reads
	std::array<char, catalog::kListingSize> _receiveBuffer{};
	std::vector<Entry> _entries{};
	//NOTE: the catalogs that ended their list this round - one past its answer limit stays silent
	std::vector<udp::endpoint> _answeredCatalogs{};
	//NOTE: whether a listed address has a route here - asked once, IsReachable opens a socket of its own
	std::map<boost::asio::ip::address, bool> _routes{};
	Clock::time_point _roundStart{};
	Clock::time_point _nextRound{};
	bool _isRoundOpen{};
};
}//namespace network
