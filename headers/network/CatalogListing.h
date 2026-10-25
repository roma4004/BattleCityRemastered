#pragma once

#include "Endpoints.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/asio/steady_timer.hpp>

namespace network
{
using boost::asio::ip::udp;

class DiscoveryBeacon;

//NOTE: keeps a server listed in the catalog by heartbeats
class CatalogListing final
{
public:
	//NOTE: sent from the server's address - the catalog lists the sender, the OS might pick another
	CatalogListing(boost::asio::io_context& ioContext, const ServerAddress& catalog,
				   const boost::asio::ip::address& server, const DiscoveryBeacon& beacon);

	~CatalogListing();

	CatalogListing(const CatalogListing&) = delete;
	CatalogListing& operator=(const CatalogListing&) = delete;
	CatalogListing(CatalogListing&&) = delete;
	CatalogListing& operator=(CatalogListing&&) = delete;

	//NOTE: resent at once, not on the next heartbeat
	void SetPortClosed(bool isPortClosed);
	//NOTE: says goodbye - the catalog drops the server at once, not after kSilenceLimit
	void Shutdown();

private:
	void Send();

	const DiscoveryBeacon& _beacon;
	udp::socket _socket;
	udp::endpoint _catalog{};
	boost::asio::steady_timer _heartbeat;
	bool _isPortClosed{};
};
}//namespace network
