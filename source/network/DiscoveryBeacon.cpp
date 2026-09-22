#include "network/DiscoveryBeacon.h"
#include "network/Discovery.h"
#include "network/Endpoints.h"
#include "utils/Log.h"
#include <boost/system/error_code.hpp>
#include <string>
#include <string_view>
#include <tuple>
#include <utility>

namespace network
{
DiscoveryBeacon::DiscoveryBeacon(boost::asio::io_context& ioContext, const boost::asio::ip::address& host,
								 const std::uint16_t gamePort, std::function<std::uint8_t()> freeSeats)
	: _socket{ioContext}
	, _gamePort{gamePort}
	, _freeSeats{std::move(freeSeats)}
{
	const udp::endpoint endpoint{host, kDiscoveryPort};

	boost::system::error_code ec;
	std::ignore = _socket.open(endpoint.protocol(), ec);
	if (!ec)
	{
		std::ignore = _socket.bind(endpoint, ec);
	}

	//NOTE: deliberately not SO_REUSEADDR - two servers answering the same probe would send a client to
	//whichever replied first, and a coin toss is worse than being told to name the port
	if (ec)
	{
		std::ignore = _socket.close(ec);
		Log::Info("DiscoveryBeacon: port " + std::to_string(kDiscoveryPort)
				  + " is taken, this server is reachable by --port only");

		return;
	}

	Receive();
}

DiscoveryBeacon::~DiscoveryBeacon() { Shutdown(); }

void DiscoveryBeacon::Shutdown()
{
	if (!_socket.is_open())
	{
		return;
	}

	boost::system::error_code ec;
	std::ignore = _socket.cancel(ec);
	std::ignore = _socket.close(ec);
}

void DiscoveryBeacon::Receive()
{
	_socket.async_receive_from(boost::asio::buffer(_receiveBuffer), _sender,
							   [this](const boost::system::error_code& ec, const std::size_t size)
							   {
								   if (ec == boost::asio::error::operation_aborted || !_socket.is_open())
								   {
									   return;
								   }

								   if (!ec && std::string_view{_receiveBuffer.data(), size} == discovery::kProbe)
								   {
									   const discovery::Reply reply{
											   .protocolVersion = discovery::kProtocolVersion,
											   .gamePort = _gamePort,
											   .freeSeats = _freeSeats ? _freeSeats() : std::uint8_t{}};

									   //NOTE: one datagram, no retry - a probe that got no answer asks again
									   boost::system::error_code sendEc;
									   const auto bytes{discovery::Pack(reply)};
									   std::ignore = _socket.send_to(boost::asio::buffer(bytes), _sender, {}, sendEc);
								   }

								   Receive();
							   });
}
}//namespace network
