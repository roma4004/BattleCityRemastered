#include "network/DiscoveryScan.h"
#include "network/Discovery.h"
#include "network/Endpoints.h"
#include "utils/Log.h"
#include <boost/asio/ip/address.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/system/error_code.hpp>
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <vector>

using namespace std::chrono_literals;

namespace
{
//NOTE: answers take milliseconds - a longer round only keeps a gone server listed
constexpr auto kRoundLength{1s};
constexpr auto kRefreshStep{10s};
}//namespace

namespace network
{
DiscoveryScan::DiscoveryScan(const std::string& ownHost)
	: _socket{_ioContext}
{
	boost::system::error_code ec;
	std::ignore = _socket.open(udp::v4(), ec);
	if (ec)
	{
		Log::Error("DiscoveryScan open: " + ec.message());

		return;
	}

	std::ignore = _socket.set_option(boost::asio::socket_base::broadcast{true}, ec);
	std::ignore = _socket.non_blocking(true, ec);

	_targets.emplace_back(boost::asio::ip::address_v4::broadcast(), kDiscoveryPort);
	if (const auto own{boost::asio::ip::make_address(ownHost, ec)}; !ec && own.is_v4())
	{
		_targets.emplace_back(own, kDiscoveryPort);
	}

	Refresh();
}

void DiscoveryScan::Refresh()
{
	if (!_socket.is_open())
	{
		return;
	}

	for (const udp::endpoint& target: _targets)
	{
		boost::system::error_code ec;
		std::ignore = _socket.send_to(boost::asio::buffer(discovery::kProbe), target, {}, ec);
	}

	std::ranges::for_each(_entries, [](Entry& entry) { entry.isAnswering = false; });
	_roundStart = Clock::now();
	_nextRound = _roundStart + kRefreshStep;
	_isRoundOpen = true;
}

bool DiscoveryScan::Poll()
{
	if (!_socket.is_open())
	{
		return false;
	}

	if (Clock::now() >= _nextRound)
	{
		Refresh();
	}

	bool isChanged{};
	for (;;)
	{
		udp::endpoint sender{};
		boost::system::error_code ec;
		const std::size_t size{_socket.receive_from(boost::asio::buffer(_receiveBuffer), sender, {}, ec)};
		if (ec)
		{
			break;
		}

		const auto reply{discovery::Parse(std::string_view{_receiveBuffer.data(), size})};
		if (!reply || reply->protocolVersion != discovery::kProtocolVersion)
		{
			continue;
		}

		const FoundServer server{.host = sender.address().to_string(),
								 .gamePort = reply->gamePort,
								 .seats = reply->seats,
								 .freeSeats = reply->freeSeats,
								 .rules = reply->rules};
		const auto isSame = [&server](const Entry& entry)
		{
			return entry.server.host == server.host && entry.server.gamePort == server.gamePort;
		};

		//NOTE: the broadcast and the direct probe may both reach one server
		if (const auto known{std::ranges::find_if(_entries, isSame)}; known != _entries.end())
		{
			isChanged = isChanged || known->server != server;
			*known = Entry{.server = server, .isAnswering = true};
		}
		else
		{
			_entries.push_back(Entry{.server = server, .isAnswering = true});
			isChanged = true;
		}
	}

	if (_isRoundOpen && Clock::now() - _roundStart >= kRoundLength)
	{
		_isRoundOpen = false;
		isChanged = std::erase_if(_entries, [](const Entry& entry) { return !entry.isAnswering; }) > 0 || isChanged;
	}

	return isChanged;
}

std::vector<FoundServer> DiscoveryScan::Servers() const
{
	return _entries | std::views::transform(&Entry::server) | std::ranges::to<std::vector>();
}
}//namespace network
