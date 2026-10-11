#include "network/Router.h"
#include "enums/RouterState.h"
#include <array>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/system/error_code.hpp>
#include <memory>
#include <miniupnpc.h>
#include <string>
#include <upnpcommands.h>
#include <upnpdev.h>

namespace
{
//NOTE: how long the network is listened to for a router - UPnP's own advice is a second or two. miniupnpc asks
//under four names in turn and stops at the first one answered, so silence takes four times as long
constexpr int kDiscoverMilliseconds{2000};
//NOTE: two hops, as UPnP advises - the router is the next one
constexpr unsigned char kDiscoverTtl{2u};

//NOTE: the provider's shared range (RFC 6598) tells a provider's NAT from a second home router, which has a private one
[[nodiscard]] RouterState ConnectedStateOf(const std::string& outsideAddress)
{
	boost::system::error_code ec;
	const auto address{boost::asio::ip::make_address_v4(outsideAddress, ec)};
	if (ec)
	{
		return RouterState::Offline;
	}

	const auto bytes{address.to_bytes()};
	if (bytes[0] == 100u && (bytes[1] & 0xC0u) == 64u)
	{
		return RouterState::BehindProvider;
	}

	const bool isPrivate{bytes[0] == 10u || (bytes[0] == 172u && (bytes[1] & 0xF0u) == 16u)
						 || (bytes[0] == 192u && bytes[1] == 168u) || (bytes[0] == 169u && bytes[1] == 254u)};

	return isPrivate ? RouterState::BehindRouter : RouterState::Online;
}
}//namespace

namespace network
{
//NOTE: Winsock, which miniupnpc needs on Windows, is started for the whole process by asio
Router::Router()
{
	int error{};
	const std::unique_ptr<UPNPDev, decltype(&freeUPNPDevlist)> devices{
			upnpDiscover(kDiscoverMilliseconds, nullptr, nullptr, UPNP_LOCAL_PORT_ANY, 0, kDiscoverTtl, &error),
			&freeUPNPDevlist};
	if (!devices)
	{
		return;
	}

	std::array<char, 64> insideAddress{};
	std::array<char, 64> outsideAddress{};
	const int igd{UPNP_GetValidIGD(devices.get(), &_urls, &_data, insideAddress.data(),
								   static_cast<int>(insideAddress.size()), outsideAddress.data(),
								   static_cast<int>(outsideAddress.size()))};
	_insideAddress = insideAddress.data();
	_outsideAddress = outsideAddress.data();
	const bool isConnected{igd == UPNP_CONNECTED_IGD || igd == UPNP_PRIVATEIP_IGD};
	_state = isConnected ? ConnectedStateOf(_outsideAddress) : RouterState::Offline;
}

Router::Router(const std::string& descriptionUrl)
{
	std::array<char, 64> insideAddress{};
	if (UPNP_GetIGDFromUrl(descriptionUrl.c_str(), &_urls, &_data, insideAddress.data(),
						   static_cast<int>(insideAddress.size()))
		== 0)
	{
		return;
	}

	_insideAddress = insideAddress.data();
	std::array<char, 64> outsideAddress{};
	const bool isConnected{UPNPIGD_IsConnected(&_urls, &_data) != 0
						   && UPNP_GetExternalIPAddress(ControlUrl(), ServiceType(), outsideAddress.data())
									  == UPNPCOMMAND_SUCCESS};
	_outsideAddress = outsideAddress.data();
	_state = isConnected ? ConnectedStateOf(_outsideAddress) : RouterState::Offline;
}

//NOTE: left empty when nothing that answered is a router, and freeing empty ones is a no-op
Router::~Router() { FreeUPNPUrls(&_urls); }

std::string Router::DescriptionUrl() const
{
	return _urls.rootdescURL != nullptr ? std::string{_urls.rootdescURL} : std::string{};
}
}//namespace network
