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
#include <cstdint>
#include <optional>
#include <random>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

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
			_ownHosts.push_back(own.to_string());
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
		//NOTE: this machine by IPv6 as well - a server on ::1 hears neither the broadcast nor the group
		_targets.emplace_back(boost::asio::ip::address_v6::loopback(), kDiscoveryPort);
		if (const auto own{boost::asio::ip::make_address(LocalIPv6Address(), ec)}; !ec && !own.is_loopback())
		{
			_targets.emplace_back(own, kDiscoveryPort);
			_ownHosts.push_back(own.to_string());
		}
	}

	std::random_device device{};
	for (const ServerAddress& catalog: catalogs)
	{
		if (const auto host{boost::asio::ip::make_address(catalog.host, ec)}; !ec)
		{
			_catalogs.push_back(AskedCatalog{.endpoint = {host, catalog.port},
											 .nonce = std::uniform_int_distribution<std::uint64_t>{}(device)});
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

	for (const auto& [endpoint, nonce]: _catalogs)
	{
		boost::system::error_code ec;
		udp::socket& socket{endpoint.address().is_v4() ? _socket : _socketIPv6};
		std::ignore = socket.send_to(boost::asio::buffer(catalog::PackQuery(nonce)), endpoint, {}, ec);
	}

	for (Heard& heard: _entries | std::views::transform(&Entry::addresses) | std::views::join)
	{
		heard.isAnswering = false;
	}

	_answeredCatalogs.clear();
	_roundStart = Clock::now();
	constexpr auto refreshStep{10s};
	_nextRound = _roundStart + refreshStep;
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
	//NOTE: answers take milliseconds - a longer round only keeps a gone server listed
	constexpr auto roundLength{1s};
	if (_isRoundOpen && now - _roundStart >= roundLength)
	{
		_isRoundOpen = false;
		//NOTE: a catalog past its answer limit said nothing - its addresses stay until their silence runs out
		const auto isGone = [this, now](const Heard& heard)
		{
			return !heard.isAnswering
				   && (!heard.catalog || std::ranges::contains(_answeredCatalogs, *heard.catalog)
					   || now - heard.heardAt > catalog::kSilenceLimit);
		};
		std::ranges::for_each(_entries, [&isGone, &isChanged](Entry& entry)
		{
			isChanged = std::erase_if(entry.addresses, isGone) > 0u || isChanged;
		});
		std::erase_if(_entries, [](const Entry& entry) { return entry.addresses.empty(); });
	}

	return isChanged;
}

bool DiscoveryScan::Read(udp::socket& socket)
{
	bool isChanged{};
	const auto isAsked = [this](const std::uint64_t nonce)
	{
		return std::ranges::contains(_catalogs, nonce, &AskedCatalog::nonce);
	};
	//NOTE: read on the game thread - a flood faster than a frame would otherwise hold the frame forever
	constexpr std::size_t datagramsPerPoll{256u};
	for (std::size_t read{}; read < datagramsPerPoll; ++read)
	{
		udp::endpoint sender{};
		boost::system::error_code ec;
		const std::size_t size{socket.receive_from(boost::asio::buffer(_receiveBuffer), sender, {}, ec)};
		if (ec)
		{
			break;
		}

		const std::string_view datagram{_receiveBuffer.data(), size};
		if (const auto end{catalog::ParseTagged(datagram, catalog::kListEnd)})
		{
			if (isAsked(*end))
			{
				_answeredCatalogs.push_back(*end);
			}

			continue;
		}

		//NOTE: a beacon answers for itself, a catalog of this scan for every server it lists - nobody else lists any
		std::optional<catalog::Listing> listing{catalog::ParseListing(datagram)};
		if (listing && !isAsked(listing->nonce))
		{
			continue;
		}

		//NOTE: each new address costs a socket to ask for its route
		const bool hasRoom{_entries.size() < catalog::kMaxServers};
		if (listing && !_routes.contains(listing->host))
		{
			if (!hasRoom)
			{
				continue;
			}

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
			if (!_routes.contains(host) && hasRoom)
			{
				_routes.emplace(host, IsReachable(host.to_string()));
			}

			listing->host = _routes.contains(host) && _routes.at(host) ? host : listing->host;
		}

		if (!listing || listing->heartbeat.reply.protocolVersion != discovery::kProtocolVersion)
		{
			continue;
		}

		const discovery::Reply& reply{listing->heartbeat.reply};
		const ServerAddress address{.host = listing->host.to_string(), .port = reply.gamePort};
		const auto addressOf{[](const Heard& heard) -> const ServerAddress& { return heard.found.address; }};
		//NOTE: an address is one server's - a server restarted there answers under a new id
		const auto isOther = [&reply](const Entry& entry) { return entry.serverId != reply.serverId; };
		for (Entry& other: _entries | std::views::filter(isOther))
		{
			const auto isThere = [&address](const Heard& heard) { return heard.found.address == address; };
			isChanged = std::erase_if(other.addresses, isThere) > 0u || isChanged;
		}
		std::erase_if(_entries, [](const Entry& entry) { return entry.addresses.empty(); });

		auto server{std::ranges::find(_entries, reply.serverId, &Entry::serverId)};
		if (server == _entries.end() && _entries.size() >= catalog::kMaxServers)
		{
			continue;
		}

		if (server == _entries.end())
		{
			server = _entries.insert(_entries.end(), Entry{.serverId = reply.serverId});
		}

		auto heard{std::ranges::find(server->addresses, address, addressOf)};
		const bool isNew{heard == server->addresses.end()};
		//NOTE: both protocols over a machine's links - more under one id would be somebody else's
		constexpr std::size_t maxAddresses{8u};
		if (isNew && server->addresses.size() >= maxAddresses)
		{
			continue;
		}

		if (isNew)
		{
			heard = server->addresses.insert(server->addresses.end(), Heard{.found = {.address = address}});
		}

		const auto now{Clock::now()};
		heard->isAnswering = true;
		heard->heardAt = now;
		heard->catalog = beaconReply ? std::nullopt : std::optional{listing->nonce};
		if (beaconReply)
		{
			heard->localHeardAt = now;
		}

		//NOTE: only while its beacon is heard - after a move to another network the LAN address leads nowhere
		const auto isLocal = [now](const Heard& local)
		{
			return local.localHeardAt && now - *local.localHeardAt <= catalog::kSilenceLimit;
		};
		const FoundAddress was{heard->found};
		heard->found = FoundAddress{.address = address,
									.seats = reply.seats,
									.freeSeats = reply.freeSeats,
									.rules = reply.rules,
									.isPortClosed = listing->heartbeat.isPortClosed && !isLocal(*heard)};
		isChanged = isNew || heard->found != was || isChanged;

		const auto reach = [this, &isLocal](const Heard& one)
		{
			const std::string& host{one.found.address.host};
			boost::system::error_code parseError;
			if (boost::asio::ip::make_address(host, parseError).is_loopback())
			{
				return std::pair{3, host.contains(':')};
			}

			return std::pair{std::ranges::contains(_ownHosts, host) ? 0 : (isLocal(one) ? 1 : 2), host.contains(':')};
		};
		if (!std::ranges::is_sorted(server->addresses, {}, reach))
		{
			std::ranges::stable_sort(server->addresses, {}, reach);
			isChanged = true;
		}
	}

	return isChanged;
}

std::vector<FoundServer> DiscoveryScan::Servers() const
{
	const auto found = [](const Entry& entry)
	{
		return FoundServer{
				.addresses = entry.addresses | std::views::transform(&Heard::found) | std::ranges::to<std::vector>()};
	};

	return _entries | std::views::transform(found) | std::ranges::to<std::vector>();
}
}//namespace network
