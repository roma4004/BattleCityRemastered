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
//NOTE: the same rule the client dials by - a server told to listen everywhere is asked on this machine
boost::asio::ip::address ProbeAddress(const std::string& host)
{
	//NOTE: one named result and one return - two returns of different objects is what -Wnrvo is about.
	//A host that does not parse leaves a v4 unspecified behind, so the fallback below is already right
	boost::system::error_code ec;
	boost::asio::ip::address address{boost::asio::ip::make_address(host, ec)};
	if (ec || address.is_unspecified())
	{
		address = address.is_v6() ? boost::asio::ip::address{boost::asio::ip::address_v6::loopback()}
								  : boost::asio::ip::address{boost::asio::ip::address_v4::loopback()};
	}

	return address;
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

	//NOTE: the answer to this probe is not here yet - what is read now is the answer to an earlier one,
	//which says the same thing, so the freshest datagram in the buffer wins
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
