#pragma once

#include "Discovery.h"
#include <array>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/udp.hpp>
#include <cstdint>
#include <functional>

namespace network
{
using boost::asio::ip::udp;

//NOTE: the server's side of "where is the game" - it sits on kDiscoveryPort, which never changes, and
//answers with the port the match actually got. Named for what it does here, not for who asks
class DiscoveryBeacon final
{
public:
	//NOTE: freeSeats is asked at answer time, not captured as a number - the answer is only worth
	//anything if it describes the server as it is when the probe arrives
	DiscoveryBeacon(boost::asio::io_context& ioContext, const boost::asio::ip::address& host,
					std::uint16_t gamePort, std::function<std::uint8_t()> freeSeats);

	~DiscoveryBeacon();

	DiscoveryBeacon(const DiscoveryBeacon&) = delete;
	DiscoveryBeacon& operator=(const DiscoveryBeacon&) = delete;
	DiscoveryBeacon(DiscoveryBeacon&&) = delete;
	DiscoveryBeacon& operator=(DiscoveryBeacon&&) = delete;

	//NOTE: false when the port was already taken - a second server on this machine is reachable by its
	//number alone, and says so instead of fighting the first one for the well-known one
	[[nodiscard]] bool IsListening() const noexcept { return _socket.is_open(); }

	void Shutdown();

private:
	void Receive();

	udp::socket _socket;
	udp::endpoint _sender{};
	//NOTE: room enough to tell a probe from anything else - a longer datagram is not one either way
	std::array<char, 16u> _receiveBuffer{};
	const std::uint16_t _gamePort;
	std::function<std::uint8_t()> _freeSeats;
};
}//namespace network
