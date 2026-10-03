#pragma once

#include "Discovery.h"
#include "enums/MatchRules.h"
#include <array>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <chrono>
#include <cstdint>
#include <string>
#include <vector>

namespace network
{
using boost::asio::ip::udp;

struct FoundServer final
{
	std::string host{};
	std::uint16_t gamePort{};
	std::uint8_t seats{};
	std::uint8_t freeSeats{};
	MatchRules rules{};

	[[nodiscard]] bool operator==(const FoundServer& rhs) const = default;
};

//NOTE: probes by broadcast and at this machine's own address - a broadcast may not return to its sender
class DiscoveryScan final
{
public:
	explicit DiscoveryScan(const std::string& ownHost);

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
		bool isAnswering{};
	};

	boost::asio::io_context _ioContext{};
	udp::socket _socket;
	std::vector<udp::endpoint> _targets{};
	std::array<char, discovery::kReplySize> _receiveBuffer{};
	std::vector<Entry> _entries{};
	Clock::time_point _roundStart{};
	Clock::time_point _nextRound{};
	bool _isRoundOpen{};
};
}//namespace network
