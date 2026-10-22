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

//NOTE: the client side of discovery - asks kDiscoveryPort for the game port; non-blocking, polled every frame
class DiscoveryProbe final
{
public:
	explicit DiscoveryProbe(const std::string& host);

	DiscoveryProbe(const DiscoveryProbe&) = delete;
	DiscoveryProbe& operator=(const DiscoveryProbe&) = delete;
	DiscoveryProbe(DiscoveryProbe&&) = delete;
	DiscoveryProbe& operator=(DiscoveryProbe&&) = delete;

	//NOTE: sends a probe and reads what arrived - empty means not yet, not nobody; another protocol version is refused
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
