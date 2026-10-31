#include "network/DiscoveryBeacon.h"
#include "network/Discovery.h"
#include "network/Endpoints.h"
#include "enums/MatchRules.h"
#include "utils/Log.h"
#include <boost/asio/ip/address_v6.hpp>
#include <boost/asio/ip/multicast.hpp>
#include <boost/system/error_code.hpp>
#include <algorithm>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <optional>
#include <random>
#include <ranges>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#include <vector>

namespace
{
//NOTE: a probe from a link-local address came over that link - the server's address on it that reaches further
std::optional<boost::asio::ip::address_v6> FurtherAddressFor(const boost::asio::ip::udp::endpoint& asker)
{
	if (!asker.address().is_v6() || !asker.address().to_v6().is_link_local())
	{
		return std::nullopt;
	}

	const std::vector<network::IPv6Interface> links{network::IPv6Interfaces()};
	const auto link{std::ranges::find(links, asker.address().to_v6().scope_id(), &network::IPv6Interface::index)};
	if (link == links.end())
	{
		return std::nullopt;
	}

	boost::system::error_code ec;
	auto furthest{boost::asio::ip::make_address_v6(link->furthest, ec)};
	if (ec || furthest.is_link_local())
	{
		return std::nullopt;
	}

	return furthest;
}
}//namespace

namespace network
{
DiscoveryBeacon::DiscoveryBeacon(boost::asio::io_context& ioContext, const boost::asio::ip::address& host,
								 const std::uint16_t gamePort, const std::uint8_t seats, const MatchRules rules,
								 std::function<std::uint8_t()> freeSeats)
	: _inbox{.socket = udp::socket{ioContext}}
	, _broadcastInbox{.socket = udp::socket{ioContext}}
	, _subnetInbox{.socket = udp::socket{ioContext}}
	, _isOnEveryAddress{host.is_unspecified()}
	, _gamePort{gamePort}
	, _seats{seats}
	, _rules{rules}
	, _serverId{std::random_device{}()}
	, _freeSeats{std::move(freeSeats)}
{
	//NOTE: no SO_REUSEADDR - two servers answering one probe would send the client to whichever replied first
	if (!Listen(_inbox, host))
	{
		Log::Info("DiscoveryBeacon: port " + std::to_string(kDiscoveryPort)
				  + " is taken, this server is reachable by --port only");

		return;
	}

	if (host.is_v6() && !host.is_loopback())
	{
		JoinGroupIPv6();
	}

#ifndef _WIN32 //NOTE: Linux
	if (host.is_v6() && !host.is_unspecified() && !host.is_loopback())
	{
		ListenToGroupIPv6(host);

		return;
	}

	//NOTE: a socket on every address hears a broadcast by itself, and none ever comes to loopback
	if (!host.is_v4() || host.is_unspecified() || host.is_loopback())
	{
		return;
	}

	const std::optional<std::string> subnet{SubnetBroadcast(host.to_string())};
	if (!Listen(_broadcastInbox, boost::asio::ip::address_v4::broadcast())
		|| (subnet && !Listen(_subnetInbox, boost::asio::ip::make_address(*subnet))))
	{
		Log::Info("DiscoveryBeacon: not every broadcast reaches this server, a probe by its address does");
	}
#endif //NOTE: Linux
}

DiscoveryBeacon::~DiscoveryBeacon() { Shutdown(); }

void DiscoveryBeacon::Shutdown()
{
	const auto close = [](udp::socket& socket)
	{
		boost::system::error_code ec;
		std::ignore = socket.cancel(ec);
		std::ignore = socket.close(ec);
	};

	for (udp::socket* const socket: {&_inbox.socket, &_broadcastInbox.socket, &_subnetInbox.socket})
	{
		close(*socket);
	}

	std::ranges::for_each(_groupInboxes, close, &Inbox::socket);
}

discovery::Reply DiscoveryBeacon::Answer() const
{
	return discovery::Reply{.protocolVersion = discovery::kProtocolVersion,
							.gamePort = _gamePort,
							.seats = _seats,
							.freeSeats = _freeSeats ? _freeSeats() : std::uint8_t{},
							.rules = _rules,
							.serverId = _serverId};
}

bool DiscoveryBeacon::Listen(Inbox& inbox, const boost::asio::ip::address& address)
{
	const udp::endpoint endpoint{address, kDiscoveryPort};
	boost::system::error_code ec;
	std::ignore = inbox.socket.open(endpoint.protocol(), ec);
	if (!ec)
	{
		std::ignore = inbox.socket.bind(endpoint, ec);
	}

	if (ec)
	{
		std::ignore = inbox.socket.close(ec);

		return false;
	}

	Receive(inbox);

	return true;
}

void DiscoveryBeacon::JoinGroupIPv6()
{
	const auto group{boost::asio::ip::make_address_v6(std::string{discovery::kGroupIPv6})};
	std::size_t joined{};
	for (const IPv6Interface& link: IPv6Interfaces())
	{
		boost::system::error_code ec;
		std::ignore = _inbox.socket.set_option(boost::asio::ip::multicast::join_group(group, link.index), ec);
		joined += ec ? 0u : 1u;
	}

	if (joined == 0u)
	{
		Log::Info("DiscoveryBeacon: no link took the IPv6 group, this server is found by its address only");
	}
}

void DiscoveryBeacon::ListenToGroupIPv6(const boost::asio::ip::address& host)
{
	const auto group{boost::asio::ip::make_address_v6(std::string{discovery::kGroupIPv6})};
	//NOTE: a link's own address answers on that link alone - the group port on the others is left to their servers
	const auto isAnsweredOn = [own = host.to_v6()](const IPv6Interface& link)
	{
		return !own.is_link_local() || link.index == own.scope_id();
	};
	for (const IPv6Interface& link: IPv6Interfaces() | std::views::filter(isAnsweredOn))
	{
		Inbox& inbox{_groupInboxes.emplace_back(Inbox{.socket = udp::socket{_inbox.socket.get_executor()}})};
		boost::system::error_code ec;
		if (Listen(inbox, boost::asio::ip::address_v6{group.to_bytes(), link.index}))
		{
			std::ignore = inbox.socket.set_option(boost::asio::ip::multicast::join_group(group, link.index), ec);
		}

		if (!inbox.socket.is_open() || ec)
		{
			Log::Info("DiscoveryBeacon: the IPv6 group does not reach this server on every link, its address does");
		}
	}
}

void DiscoveryBeacon::Receive(Inbox& inbox)
{
	const auto onReceived = [this, &inbox](const boost::system::error_code& ec, const std::size_t size)
	{
		if (ec == boost::asio::error::operation_aborted || !inbox.socket.is_open())
		{
			return;
		}

		if (!ec && std::string_view{inbox.buffer.data(), size} == discovery::kProbe)
		{
			//NOTE: one datagram, no retry - a probe that got no answer asks again
			boost::system::error_code sendEc;
			if (const auto further{_isOnEveryAddress ? FurtherAddressFor(inbox.sender) : std::nullopt})
			{
				const auto bytes{discovery::Pack(Answer(), further->to_bytes())};
				std::ignore = _inbox.socket.send_to(boost::asio::buffer(bytes), inbox.sender, {}, sendEc);
			}
			else
			{
				const auto bytes{discovery::Pack(Answer())};
				std::ignore = _inbox.socket.send_to(boost::asio::buffer(bytes), inbox.sender, {}, sendEc);
			}
		}

		Receive(inbox);
	};
	inbox.socket.async_receive_from(boost::asio::buffer(inbox.buffer), inbox.sender, onReceived);
}
}//namespace network
