#include "network/PublicAddressProbe.h"
#include "network/Stun.h"
#include "utils/Log.h"
#include <algorithm>
#include <array>
#include <boost/asio/buffer.hpp>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/system/error_code.hpp>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <random>
#include <string>
#include <thread>
#include <tuple>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace
{
//NOTE: a datagram may be lost on the way, so the question goes again until an answer comes or time is up
constexpr auto kResendStep{500ms};
constexpr auto kPatience{5s};

[[nodiscard]] network::stun::Transaction RandomTransaction()
{
	std::random_device random{};
	std::uniform_int_distribution<int> byte{0, 255};
	network::stun::Transaction transaction{};
	std::ranges::generate(transaction, [&random, &byte] { return static_cast<std::uint8_t>(byte(random)); });

	return transaction;
}

//NOTE: blocking - a name lookup can take seconds, more with a DNS server out of reach
[[nodiscard]] std::vector<network::udp::endpoint> ResolveServers()
{
	boost::asio::io_context ioContext{};
	network::udp::resolver resolver{ioContext};
	std::vector<network::udp::endpoint> servers{};
	for (const network::stun::Server& server: network::stun::kPublicServers)
	{
		boost::system::error_code ec;
		const auto found{resolver.resolve(network::udp::v4(), server.host, server.service, ec)};
		if (!ec && !found.empty())
		{
			servers.push_back(found.begin()->endpoint());
		}
	}

	return servers;
}
}//namespace

namespace network
{
PublicAddressProbe::PublicAddressProbe()
	: _handover{std::make_shared<Handover>()}
	, _socket{_ioContext}
	, _transaction{RandomTransaction()}
{
	//NOTE: let go of - a DNS server out of reach holds it for seconds, and closing the game does not wait for that.
	//asio's own asynchronous lookup would make whoever drops the probe wait for it instead
	std::thread{[handover = _handover]
	{
		std::vector<udp::endpoint> servers{ResolveServers()};
		const std::scoped_lock lock{handover->mutex};
		handover->servers = std::move(servers);
		handover->isDone = true;
	}}.detach();
}

std::optional<std::string> PublicAddressProbe::Poll()
{
	if (_handover)
	{
		TakeHandover();
	}

	if (!_isAsking || _handover)
	{
		return _answer;
	}

	for (;;)
	{
		udp::endpoint sender{};
		boost::system::error_code ec;
		const std::size_t size{_socket.receive_from(boost::asio::buffer(_receiveBuffer), sender, {}, ec)};
		if (ec)
		{
			break;
		}

		//NOTE: only a server that was asked is believed
		if (!std::ranges::contains(_servers, sender))
		{
			continue;
		}

		if (std::optional<std::string> mapped{stun::MappedAddress({_receiveBuffer.data(), size}, _transaction)})
		{
			_answer = std::move(mapped);
			_isAsking = false;
			Log::Info("the internet sees this machine as " + *_answer);

			return _answer;
		}
	}

	const auto now{Clock::now()};
	if (now >= _giveUpAt)
	{
		_isAsking = false;
		Log::Info("no STUN server answered - the internet's address of this machine is unknown");

		return _answer;
	}

	if (now >= _nextSend)
	{
		Send(now);
	}

	return _answer;
}

void PublicAddressProbe::TakeHandover()
{
	{
		const std::scoped_lock lock{_handover->mutex};
		if (!_handover->isDone)
		{
			return;
		}

		_servers = std::move(_handover->servers);
	}

	_handover.reset();
	if (_servers.empty())
	{
		_isAsking = false;
		Log::Info("no STUN server's name resolves - the internet's address of this machine is unknown");

		return;
	}

	boost::system::error_code ec;
	std::ignore = _socket.open(udp::v4(), ec);
	if (ec)
	{
		Log::Error("PublicAddressProbe open: " + ec.message());
		_isAsking = false;

		return;
	}

	std::ignore = _socket.non_blocking(true, ec);
	_giveUpAt = Clock::now() + kPatience;
}

void PublicAddressProbe::Send(const Clock::time_point now)
{
	_nextSend = now + kResendStep;
	const auto request{stun::Request(_transaction)};
	for (const udp::endpoint& server: _servers)
	{
		boost::system::error_code ec;
		std::ignore = _socket.send_to(boost::asio::buffer(request), server, {}, ec);
	}
}
}//namespace network
