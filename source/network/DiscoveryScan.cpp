#include "network/DiscoveryScan.h"
#include "network/Catalog.h"
#include "network/Discovery.h"
#include "network/Endpoints.h"
#include "utils/Log.h"
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v6.hpp>
#include <boost/asio/ip/multicast.hpp>
#include <boost/asio/socket_base.hpp>
#include <boost/system/error_code.hpp>
#include <algorithm>
#include <chrono>
#include <cstddef>
#include <iterator>
#include <optional>
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
DiscoveryScan::DiscoveryScan(const std::string& ownHost, const std::vector<ServerAddress>& catalogs)
	: _socket{_ioContext}
	, _socketIPv6{_ioContext}
{
	boost::system::error_code ec;
	std::ignore = _socket.open(udp::v4(), ec);
	if (ec)
	{
		Log::Error("DiscoveryScan open: " + ec.message());
	}
	else
	{
		std::ignore = _socket.set_option(boost::asio::socket_base::broadcast{true}, ec);
		std::ignore = _socket.non_blocking(true, ec);

		_targets.emplace_back(boost::asio::ip::address_v4::broadcast(), kDiscoveryPort);
		std::ranges::transform(LocalBroadcasts(), std::back_inserter(_targets), [](const std::string& broadcast)
		{
			return udp::endpoint{boost::asio::ip::make_address_v4(broadcast), kDiscoveryPort};
		});
		if (const auto own{boost::asio::ip::make_address(ownHost, ec)}; !ec && own.is_v4())
		{
			_targets.emplace_back(own, kDiscoveryPort);
		}
	}

	//NOTE: fails without IPv6 - the catalog is then asked over IPv4 alone
	std::ignore = _socketIPv6.open(udp::v6(), ec);
	std::ignore = _socketIPv6.non_blocking(true, ec);
	if (_socketIPv6.is_open())
	{
		const auto group{boost::asio::ip::make_address_v6(std::string{discovery::kGroupIPv6})};
		std::ranges::transform(IPv6Interfaces(), std::back_inserter(_targets), [&group](const IPv6Interface& link)
		{
			return udp::endpoint{boost::asio::ip::address_v6{group.to_bytes(), link.index}, kDiscoveryPort};
		});
	}

	for (const ServerAddress& catalog: catalogs)
	{
		if (const auto host{boost::asio::ip::make_address(catalog.host, ec)}; !ec)
		{
			_targets.emplace_back(host, catalog.port);
		}
	}

	Refresh();
}

void DiscoveryScan::Refresh()
{
	if (!_socket.is_open() && !_socketIPv6.is_open())
	{
		return;
	}

	for (const udp::endpoint& target: _targets)
	{
		boost::system::error_code ec;
		udp::socket& socket{target.address().is_v4() ? _socket : _socketIPv6};
		//NOTE: a link-local group is on every link at once - the interface says which one the probe goes out on
		if (target.address().is_multicast())
		{
			const auto link{target.address().to_v6().scope_id()};
			std::ignore = socket.set_option(boost::asio::ip::multicast::outbound_interface{link}, ec);
		}

		std::ignore = socket.send_to(boost::asio::buffer(discovery::kProbe), target, {}, ec);
	}

	std::ranges::for_each(_entries, [](Entry& entry) { entry.isAnswering = false; });
	_answeredCatalogs.clear();
	_roundStart = Clock::now();
	_nextRound = _roundStart + kRefreshStep;
	_isRoundOpen = true;
}

bool DiscoveryScan::Poll()
{
	if (!_socket.is_open() && !_socketIPv6.is_open())
	{
		return false;
	}

	if (Clock::now() >= _nextRound)
	{
		Refresh();
	}

	bool isChanged{Read(_socket)};
	isChanged = Read(_socketIPv6) || isChanged;
	const auto now{Clock::now()};
	if (_isRoundOpen && now - _roundStart >= kRoundLength)
	{
		_isRoundOpen = false;
		//NOTE: a catalog past its answer limit said nothing - its servers stay until their silence runs out
		const auto isGone = [this, now](const Entry& entry)
		{
			return !entry.isAnswering
				   && (!entry.catalog || std::ranges::contains(_answeredCatalogs, *entry.catalog)
					   || now - entry.heardAt > catalog::kSilenceLimit);
		};
		isChanged = std::erase_if(_entries, isGone) > 0 || isChanged;
	}

	return isChanged;
}

bool DiscoveryScan::Read(udp::socket& socket)
{
	bool isChanged{};
	for (;;)
	{
		udp::endpoint sender{};
		boost::system::error_code ec;
		const std::size_t size{socket.receive_from(boost::asio::buffer(_receiveBuffer), sender, {}, ec)};
		if (ec)
		{
			break;
		}

		const std::string_view datagram{_receiveBuffer.data(), size};
		if (datagram == catalog::kListEnd)
		{
			_answeredCatalogs.push_back(sender);

			continue;
		}

		//NOTE: a beacon answers for itself, the catalog for every server it lists
		std::optional<catalog::Listing> listing{catalog::ParseListing(datagram)};
		if (listing && !_routes.contains(listing->host))
		{
			_routes.emplace(listing->host, IsReachable(listing->host.to_string()));
		}

		//NOTE: no route over its protocol - listed by the catalog, but cannot be joined
		if (listing && !_routes.at(listing->host))
		{
			continue;
		}

		const auto beaconReply{discovery::Parse(datagram)};
		if (beaconReply)
		{
			listing = catalog::Listing{.heartbeat = {.reply = *beaconReply}, .host = sender.address()};
		}

		//NOTE: the link's own address works from this machine alone - the further one, if there is a route to it
		if (const auto further{discovery::FurthestOf(datagram)}; beaconReply && further)
		{
			const boost::asio::ip::address host{boost::asio::ip::address_v6{*further}};
			if (!_routes.contains(host))
			{
				_routes.emplace(host, IsReachable(host.to_string()));
			}

			listing->host = _routes.at(host) ? host : listing->host;
		}

		if (!listing || listing->heartbeat.reply.protocolVersion != discovery::kProtocolVersion)
		{
			continue;
		}

		const discovery::Reply& reply{listing->heartbeat.reply};
		const ServerAddress address{.host = listing->host.to_string(), .port = reply.gamePort};
		//NOTE: the broadcast, the direct probe and the catalog may all reach one server
		const auto addressOf{[](const Entry& entry) -> const ServerAddress& { return entry.server.address; }};
		auto known{std::ranges::find(_entries, address, addressOf)};
		//NOTE: a beacon and the catalog may tell of one server under two addresses - a LAN one and the router's
		const auto isToldByTheOther = [&reply, &beaconReply](const Entry& entry)
		{
			return entry.serverId == reply.serverId && entry.isLocal != beaconReply.has_value();
		};
		if (known == _entries.end())
		{
			known = std::ranges::find_if(_entries, isToldByTheOther);
		}

		const bool isKnownLocal{known != _entries.end() && known->isLocal};
		const bool isLocal{beaconReply.has_value() || isKnownLocal};
		//NOTE: the address a beacon answered from stays - the server is joined there without the router
		const FoundServer server{.address = isKnownLocal ? known->server.address : address,
								 .seats = reply.seats,
								 .freeSeats = reply.freeSeats,
								 .rules = reply.rules,
								 .isPortClosed = listing->heartbeat.isPortClosed && !isLocal};
		const Entry entry{.server = server,
						  .serverId = reply.serverId,
						  .isAnswering = true,
						  .isLocal = isLocal,
						  .catalog = beaconReply ? std::nullopt : std::optional{sender},
						  .heardAt = Clock::now()};
		if (known != _entries.end())
		{
			isChanged = isChanged || known->server != server;
			*known = entry;
		}
		else
		{
			_entries.push_back(entry);
			isChanged = true;
		}
	}

	return isChanged;
}

std::vector<FoundServer> DiscoveryScan::Servers() const
{
	return _entries | std::views::transform(&Entry::server) | std::ranges::to<std::vector>();
}
}//namespace network
