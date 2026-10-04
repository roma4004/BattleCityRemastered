#include "network/PortMapping.h"
#include "components/events/CoreLifecycleEvents.h"
#include "enums/PortForwarding.h"
#include "network/Endpoints.h"
#include "network/PublicAddressProbe.h"
#include "network/Router.h"
#include "utils/Log.h"
#include <array>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/address.hpp>
#include <boost/asio/ip/address_v4.hpp>
#include <boost/asio/ip/udp.hpp>
#include <boost/system/error_code.hpp>
#include <chrono>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <string_view>
#include <tuple>
#include <upnpcommands.h>
#include <upnperrors.h>
#include <utility>
#include <vector>

using namespace std::chrono_literals;

namespace
{
constexpr auto kDescription{"Battle City Remastered"};
//NOTE: renewed at half, so the port stays open while the server runs - and one killed outright leaves it open
//no longer than this
constexpr auto kLease{20min};
//NOTE: a router that keeps a port only for good answers so, and is asked again with no end set
constexpr int kOnlyPermanentLeases{725};
//NOTE: a port kept for good needs no renewal - asked again once a day all the same, in case the router forgot it
constexpr auto kPermanentRenewStep{24h};
constexpr auto kInternetPollStep{50ms};
//NOTE: a router lists every mapping of every machine behind it - a long list is not walked to its end
constexpr int kMappingsRead{128};

[[nodiscard]] bool IsEveryNetwork(const std::string& host)
{
	boost::system::error_code ec;
	const auto address{boost::asio::ip::make_address(host, ec)};

	return !ec && address.is_unspecified();
}

//NOTE: nothing on this machine listens there - Windows lets a socket on every network in beside one on a single
//address, so both are tried
[[nodiscard]] bool IsPortFree(const std::string& host, const std::uint16_t port)
{
	boost::asio::io_context ioContext{};
	const auto canBind = [&ioContext, port](const boost::asio::ip::address& address)
	{
		boost::asio::ip::udp::socket socket{ioContext};
		boost::system::error_code ec;
		std::ignore = socket.open(boost::asio::ip::udp::v4(), ec);
		std::ignore = socket.bind(boost::asio::ip::udp::endpoint{address, port}, ec);

		return !ec;
	};

	boost::system::error_code ec;
	const auto address{boost::asio::ip::make_address(host, ec)};

	return !ec && canBind(address) && canBind(boost::asio::ip::address_v4::any());
}

//NOTE: what a server of ours left open before - killed outright, or on a router that keeps a port for good. Only
//ours by the description, only to this machine, and only a port nobody here listens on: a second server on this
//machine keeps its own. Read whole before anything goes, since a removal renumbers the list
void CloseLeftovers(const network::Router& router, const std::string& client)
{
	std::vector<std::string> leftovers{};
	for (int index{}; index < kMappingsRead; ++index)
	{
		std::array<char, 6> externalPort{};
		std::array<char, 16> internalClient{};
		std::array<char, 6> internalPort{};
		std::array<char, 4> protocol{};
		std::array<char, 80> description{};
		std::array<char, 4> enabled{};
		std::array<char, 64> remoteHost{};
		std::array<char, 16> lease{};
		if (UPNP_GetGenericPortMappingEntry(router.ControlUrl(), router.ServiceType(), std::to_string(index).c_str(),
											externalPort.data(), internalClient.data(), internalPort.data(),
											protocol.data(), description.data(), enabled.data(), remoteHost.data(),
											lease.data())
			!= UPNPCOMMAND_SUCCESS)
		{
			break;
		}

		const bool isOurs{std::string_view{description.data()} == kDescription
						  && std::string_view{protocol.data()} == "UDP" && client == internalClient.data()};
		const std::optional<std::uint16_t> port{network::ParsePort(internalPort.data())};
		if (isOurs && port && IsPortFree(client, *port))
		{
			leftovers.emplace_back(externalPort.data());
		}
	}

	for (const std::string& port: leftovers)
	{
		std::ignore = UPNP_DeletePortMapping(router.ControlUrl(), router.ServiceType(), port.c_str(), "UDP", nullptr);
		Log::Info("UPnP: UDP port " + port + ", left open on the router by an earlier server, is closed");
	}
}

//NOTE: miniupnpc names the codes it knows and returns nothing for the rest
[[nodiscard]] std::string ErrorText(const int code)
{
	const char* const text{strupnperror(code)};

	return text != nullptr ? std::string{text} : "error " + std::to_string(code);
}
}//namespace

namespace network
{
PortMapping::PortMapping(std::string host, const std::uint16_t port)
	: _host{std::move(host)}
	, _port{port}
	, _publicProbe{std::make_unique<PublicAddressProbe>()}
	, _change{PortForwardingChangedEvent{.state = PortForwarding::Opening, .port = port}}
	, _worker{[this](const std::stop_token& stop) { Forward(stop); }}
{
}

PortMapping::~PortMapping() = default;

std::optional<PortForwardingChangedEvent> PortMapping::Poll()
{
	const std::scoped_lock lock{_mutex};

	return std::exchange(_change, std::nullopt);
}

void PortMapping::Report(const PortForwarding state, std::string host)
{
	const std::scoped_lock lock{_mutex};
	_change = PortForwardingChangedEvent{.state = state, .host = std::move(host), .port = _port};
}

bool PortMapping::Sleep(const std::stop_token& stop, const std::chrono::milliseconds duration)
{
	std::unique_lock lock{_mutex};
	std::ignore = _wake.wait_for(lock, stop, duration, [] { return false; });

	return !stop.stop_requested();
}

std::optional<std::string> PortMapping::AskInternet(const std::stop_token& stop)
{
	std::optional<std::string> seen{_publicProbe->Poll()};
	while (!seen && _publicProbe->IsAsking() && Sleep(stop, kInternetPollStep))
	{
		seen = _publicProbe->Poll();
	}

	return seen;
}

void PortMapping::Forward(const std::stop_token& stop)
{
	const std::string port{std::to_string(_port)};

	const Router router{};
	if (router.GetState() == Router::State::Silent)
	{
		Log::Info("UPnP: no router answered - UDP port " + port + " has to be forwarded on it by hand");
		Report(PortForwarding::NoRouter);

		return;
	}

	if (router.GetState() == Router::State::Offline)
	{
		Log::Info("UPnP: nothing on the network is a router connected to the internet - UDP port " + port
				  + " has to be forwarded by hand");
		Report(PortForwarding::NoRouter);

		return;
	}

	if (router.GetState() == Router::State::BehindProvider)
	{
		Log::Info("UPnP: the router's outside address " + router.OutsideAddress()
				  + " is a private one - another NAT stands between it and the internet, nobody gets in from there");
		Report(PortForwarding::BehindProvider);

		return;
	}

	if (stop.stop_requested())
	{
		return;
	}

	//NOTE: the router passes the port on to whatever address it is told - this machine's on its network
	const std::string client{IsEveryNetwork(_host) ? router.InsideAddress() : _host};
	bool isPermanent{};
	const auto open = [&router, &port, &client, &isPermanent]
	{
		const std::string lease{isPermanent ? "0" : std::to_string(std::chrono::seconds{kLease}.count())};

		return UPNP_AddPortMapping(router.ControlUrl(), router.ServiceType(), port.c_str(), port.c_str(),
								   client.c_str(), kDescription, "UDP", nullptr, lease.c_str());
	};

	CloseLeftovers(router, client);

	int result{open()};
	if (result == kOnlyPermanentLeases)
	{
		isPermanent = true;
		result = open();
	}

	if (result != UPNPCOMMAND_SUCCESS)
	{
		Log::Info("UPnP: the router refused UDP port " + port + " - " + ErrorText(result));
		Report(PortForwarding::Refused);

		return;
	}

	//NOTE: the router's own address is checked against what the internet sees, and stands in when nobody answered
	if (const std::optional<std::string> seen{AskInternet(stop)}; seen && *seen != router.OutsideAddress())
	{
		Log::Info("UPnP: the router says it is " + router.OutsideAddress() + " on the internet, but the internet sees "
				  + *seen + " - a provider's NAT stands between, nobody gets in from there");
		Report(PortForwarding::BehindProvider);
	}
	else
	{
		Log::Info("UPnP: UDP port " + port + " is open on the router, the internet reaches this server at "
				  + router.OutsideAddress() + ':' + port);
		Report(PortForwarding::Open, router.OutsideAddress());
	}

	while (Sleep(stop, isPermanent ? kPermanentRenewStep : kLease / 2))
	{
		if (const int renewed{open()}; renewed != UPNPCOMMAND_SUCCESS)
		{
			Log::Error("UPnP: the router did not renew UDP port " + port + " - " + ErrorText(renewed));
		}
	}

	std::ignore = UPNP_DeletePortMapping(router.ControlUrl(), router.ServiceType(), port.c_str(), "UDP", nullptr);
	Log::Info("UPnP: UDP port " + port + " is closed on the router again");
}
}//namespace network
