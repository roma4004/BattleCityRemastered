#include "network/DiscoveryBeacon.h"
#include "network/Discovery.h"
#include "network/Endpoints.h"
#include "enums/MatchRules.h"
#include "utils/Log.h"
#include <boost/system/error_code.hpp>
#include <cstddef>
#include <cstdint>
#include <functional>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>
#ifndef _WIN32 //NOTE: Linux
#include <optional>
#endif //NOTE: Linux

namespace network
{
DiscoveryBeacon::DiscoveryBeacon(boost::asio::io_context& ioContext, const boost::asio::ip::address& host,
								 const std::uint16_t gamePort, const std::uint8_t seats, const MatchRules rules,
								 std::function<std::uint8_t()> freeSeats)
	: _inbox{.socket = udp::socket{ioContext}}
	, _broadcastInbox{.socket = udp::socket{ioContext}}
	, _subnetInbox{.socket = udp::socket{ioContext}}
	, _gamePort{gamePort}
	, _seats{seats}
	, _rules{rules}
	, _freeSeats{std::move(freeSeats)}
{
	//NOTE: deliberately not SO_REUSEADDR - two servers answering the same probe would send a client to
	//whichever replied first, and a coin toss is worse than being told to name the port
	if (!Listen(_inbox, host))
	{
		Log::Info("DiscoveryBeacon: port " + std::to_string(kDiscoveryPort)
				  + " is taken, this server is reachable by --port only");

		return;
	}

#ifndef _WIN32 //NOTE: Linux
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
	for (udp::socket* const socket: {&_inbox.socket, &_broadcastInbox.socket, &_subnetInbox.socket})
	{
		boost::system::error_code ec;
		std::ignore = socket->cancel(ec);
		std::ignore = socket->close(ec);
	}
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
			const discovery::Reply reply{.protocolVersion = discovery::kProtocolVersion,
										 .gamePort = _gamePort,
										 .seats = _seats,
										 .freeSeats = _freeSeats ? _freeSeats() : std::uint8_t{},
										 .rules = _rules};

			//NOTE: one datagram, no retry - a probe that got no answer asks again
			boost::system::error_code sendEc;
			const auto bytes{discovery::Pack(reply)};
			std::ignore = _inbox.socket.send_to(boost::asio::buffer(bytes), inbox.sender, {}, sendEc);
		}

		Receive(inbox);
	};
	inbox.socket.async_receive_from(boost::asio::buffer(inbox.buffer), inbox.sender, onReceived);
}
}//namespace network
