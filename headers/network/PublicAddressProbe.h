#pragma once

#include "Stun.h"
#include <array>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <chrono>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <vector>

namespace network
{
using boost::asio::ip::udp;

//NOTE: what the internet sees this machine as, asked of public STUN servers. Polled like DiscoveryProbe -
//nothing here waits: the servers' names are resolved on a thread of its own and every poll reads what has come
class PublicAddressProbe final
{
public:
	PublicAddressProbe();

	PublicAddressProbe(const PublicAddressProbe&) = delete;
	PublicAddressProbe& operator=(const PublicAddressProbe&) = delete;
	PublicAddressProbe(PublicAddressProbe&&) = delete;
	PublicAddressProbe& operator=(PublicAddressProbe&&) = delete;

	//NOTE: the IPv4 a server saw - empty while it is still being asked, and for good once nobody answered
	[[nodiscard]] std::optional<std::string> Poll();

	[[nodiscard]] bool IsAsking() const noexcept { return _isAsking; }

private:
	using Clock = std::chrono::steady_clock;

	//NOTE: what the thread hands over - shared, as nobody waits for the thread
	struct Handover
	{
		std::mutex mutex{};
		bool isDone{};
		std::vector<udp::endpoint> servers{};
	};

	void TakeHandover();
	void Send(Clock::time_point now);

	//NOTE: only until the thread is done
	std::shared_ptr<Handover> _handover{};
	boost::asio::io_context _ioContext{};
	udp::socket _socket;
	std::vector<udp::endpoint> _servers{};
	stun::Transaction _transaction{};
	std::array<char, 512> _receiveBuffer{};
	Clock::time_point _giveUpAt{};
	Clock::time_point _nextSend{};
	std::optional<std::string> _answer{};
	bool _isAsking{true};
};
}//namespace network
