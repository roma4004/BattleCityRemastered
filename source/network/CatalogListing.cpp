#include "network/CatalogListing.h"
#include "network/Catalog.h"
#include "network/Discovery.h"
#include "network/DiscoveryBeacon.h"
#include "network/Endpoints.h"
#include "utils/Log.h"
#include <boost/asio/buffer.hpp>
#include <boost/asio/error.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/system/error_code.hpp>
#include <string>
#include <tuple>

namespace network
{
CatalogListing::CatalogListing(boost::asio::io_context& ioContext, const ServerAddress& catalog,
							   const boost::asio::ip::address& server, const DiscoveryBeacon& beacon)
	: _beacon{beacon}
	, _socket{ioContext}
	, _heartbeat{ioContext}
{
	boost::system::error_code ec;
	const auto host{boost::asio::ip::make_address(catalog.host, ec)};
	//NOTE: the other protocol would list an address the server does not listen on
	if (!ec && host.is_v4() != server.is_v4())
	{
		ec = boost::asio::error::address_family_not_supported;
	}

	if (!ec)
	{
		_catalog = udp::endpoint{host, catalog.port};
		std::ignore = _socket.open(_catalog.protocol(), ec);
	}

	if (!ec)
	{
		std::ignore = _socket.bind(udp::endpoint{server, kAnyFreePort}, ec);
	}

	if (ec)
	{
		Log::Error("catalog " + catalog.host + ':' + std::to_string(catalog.port) + " unreachable: " + ec.message()
				   + ", players from the internet will not find this server in the list");
		std::ignore = _socket.close(ec);

		return;
	}

	Send();
}

// NOLINTNEXTLINE(bugprone-exception-escape) - cancel() throws only on an error the timer service never sets
CatalogListing::~CatalogListing() { Shutdown(); }

void CatalogListing::SetPortClosed(const bool isPortClosed)
{
	if (isPortClosed == _isPortClosed || !_socket.is_open())
	{
		return;
	}

	_isPortClosed = isPortClosed;
	Send();
}

void CatalogListing::Shutdown()
{
	std::ignore = _heartbeat.cancel();
	if (!_socket.is_open())
	{
		return;
	}

	//NOTE: if lost, the catalog drops the server after kSilenceLimit anyway
	catalog::Heartbeat goodbye{.reply = _beacon.Answer(), .isPortClosed = _isPortClosed};
	goodbye.reply.seats = 0u;
	goodbye.reply.freeSeats = 0u;
	const auto bytes{catalog::Pack(goodbye)};
	boost::system::error_code ec;
	std::ignore = _socket.send_to(boost::asio::buffer(bytes), _catalog, {}, ec);
	std::ignore = _socket.close(ec);
}

//NOTE: expires_after cancels the pending wait - an early heartbeat restarts the step
void CatalogListing::Send()
{
	//NOTE: no retry - the next heartbeat makes up for a lost one
	const auto bytes{catalog::Pack(catalog::Heartbeat{.reply = _beacon.Answer(), .isPortClosed = _isPortClosed})};
	boost::system::error_code ec;
	std::ignore = _socket.send_to(boost::asio::buffer(bytes), _catalog, {}, ec);

	_heartbeat.expires_after(catalog::kHeartbeatStep);
	_heartbeat.async_wait([this](const boost::system::error_code& waitEc)
	{
		if (!waitEc && _socket.is_open())
		{
			Send();
		}
	});
}
}//namespace network
