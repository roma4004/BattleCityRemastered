#pragma once

#include "Discovery.h"
#include <array>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <cstdint>
#include <optional>
#include <string>

namespace network
{
using boost::asio::ip::udp;

//NOTE: the client's side of "where is the game" - it asks kDiscoveryPort and reads the game port out of
//the answer. Its socket is non-blocking and it owns its own io_context, because nothing here waits: the
//caller is the game loop, which asks again next frame
class DiscoveryProbe final
{
public:
	explicit DiscoveryProbe(const std::string& host);

	DiscoveryProbe(const DiscoveryProbe&) = delete;
	DiscoveryProbe& operator=(const DiscoveryProbe&) = delete;
	DiscoveryProbe(DiscoveryProbe&&) = delete;
	DiscoveryProbe& operator=(DiscoveryProbe&&) = delete;

	//NOTE: sends one probe and reads whatever has already arrived - empty means "not yet", not "nobody
	//there". A reply from a server speaking another protocol version is refused rather than dialled
	[[nodiscard]] std::optional<discovery::Reply> Poll();

private:
	boost::asio::io_context _ioContext{};
	udp::socket _socket;
	udp::endpoint _beacon;
	std::array<char, discovery::kReplySize> _receiveBuffer{};
	//NOTE: said once - the probe repeats every poll, and so would the complaint
	bool _isVersionMismatchReported{};
};
}//namespace network
