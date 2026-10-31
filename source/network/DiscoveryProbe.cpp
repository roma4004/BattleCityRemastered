#include "network/DiscoveryProbe.h"
#include "network/Discovery.h"
#include "network/Endpoints.h"
#include "utils/Log.h"
#include <boost/asio/ip/address.hpp>
#include <boost/system/error_code.hpp>
#include <string>
#include <string_view>
#include <tuple>

namespace
{
//NOTE: dialled as the client dials; an unparsable host is asked on this machine
boost::asio::ip::address ProbeAddress(const std::string& host)
{
	boost::system::error_code ec;
	const auto address{boost::asio::ip::make_address(network::DialHost(host), ec)};

	return ec ? boost::asio::ip::address{boost::asio::ip::address_v4::loopback()} : address;
}
}//namespace

namespace network
{
DiscoveryProbe::DiscoveryProbe(const std::string& host)
	: _socket{_ioContext}
	, _beacon{ProbeAddress(host), kDiscoveryPort}
{
	boost::system::error_code ec;
	std::ignore = _socket.open(_beacon.protocol(), ec);
	if (ec)
	{
		Log::Error("DiscoveryProbe open: " + ec.message());

		return;
	}

	std::ignore = _socket.non_blocking(true, ec);
}

std::optional<discovery::Reply> DiscoveryProbe::Poll()
{
	//NOTE: one object for every return, or -Wnrvo
	std::optional<discovery::Reply> newest{};
	if (!_socket.is_open())
	{
		return newest;
	}

	boost::system::error_code ec;
	std::ignore = _socket.send_to(boost::asio::buffer(discovery::kProbe), _beacon, {}, ec);

	//NOTE: the freshest datagram wins - what arrives now answers an earlier probe, which says the same
	for (;;)
	{
		udp::endpoint sender{};
		const std::size_t size{_socket.receive_from(boost::asio::buffer(_receiveBuffer), sender, {}, ec)};
		if (ec)
		{
			break;
		}

		if (const auto reply{discovery::Parse(std::string_view{_receiveBuffer.data(), size})})
		{
			newest = reply;
		}
	}

	if (newest && newest->protocolVersion != discovery::kProtocolVersion)
	{
		if (!_isVersionMismatchReported)
		{
			_isVersionMismatchReported = true;
			Log::Error("the server next door speaks protocol " + std::to_string(newest->protocolVersion)
					   + ", this build speaks " + std::to_string(discovery::kProtocolVersion));
		}

		newest.reset();
	}

	return newest;
}
}//namespace network
