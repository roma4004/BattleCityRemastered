#include "network/Router.h"
#include <array>
#include <memory>
#include <miniupnpc.h>
#include <upnpdev.h>

namespace
{
//NOTE: how long the network is listened to for a router - UPnP's own advice is a second or two. miniupnpc asks
//under four names in turn and stops at the first one answered, so silence takes four times as long
constexpr int kDiscoverMilliseconds{2000};
//NOTE: two hops, as UPnP advises - the router is the next one
constexpr unsigned char kDiscoverTtl{2u};
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
	_state = igd == UPNP_CONNECTED_IGD ? State::Online
			 : igd == UPNP_PRIVATEIP_IGD ? State::BehindProvider : State::Offline;
}

//NOTE: left empty when nothing that answered is a router, and freeing empty ones is a no-op
Router::~Router() { FreeUPNPUrls(&_urls); }
}//namespace network
