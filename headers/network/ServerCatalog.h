#pragma once

#include "Catalog.h"
#include "Endpoints.h"
#include <array>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <chrono>
#include <cstdint>
#include <map>
#include <string_view>

namespace network
{
using boost::asio::ip::udp;

//NOTE: lists servers while their heartbeats come and answers a probe with all of them
class ServerCatalog final
{
public:
	ServerCatalog(boost::asio::io_context& ioContext, const ServerAddress& address);

	~ServerCatalog();

	ServerCatalog(const ServerCatalog&) = delete;
	ServerCatalog& operator=(const ServerCatalog&) = delete;
	ServerCatalog(ServerCatalog&&) = delete;
	ServerCatalog& operator=(ServerCatalog&&) = delete;

	//NOTE: zero when the address is taken, or is not this machine's
	[[nodiscard]] std::uint16_t BoundPort() const noexcept { return _boundPort; }

	void Shutdown();

private:
	using Clock = std::chrono::steady_clock;

	struct Entry final
	{
		catalog::Heartbeat heartbeat{};
		Clock::time_point heardAt{};
	};

	//NOTE: lists sent to an address since its window opened
	struct Answers final
	{
		Clock::time_point since{};
		std::uint8_t count{};
	};

	void Receive();
	//NOTE: forgets the silent servers and the closed windows on a timer - a flood of datagrams does not walk them
	void Sweep();
	void Take(std::string_view datagram, Clock::time_point now);

	udp::socket _socket;
	boost::asio::steady_timer _sweep;
	std::uint16_t _boundPort{};
	udp::endpoint _sender{};
	//NOTE: a byte past a heartbeat - Linux cuts a longer datagram to fit, and a cut one must not pass for it
	std::array<char, catalog::kHeartbeatSize + 1u> _buffer{};
	//NOTE: by the heartbeat's sender with the game port it named
	std::map<udp::endpoint, Entry> _entries{};
	std::map<boost::asio::ip::address, Answers> _answers{};
};
}//namespace network
