#include "network/PortMapping.h"
#include "components/events/CoreLifecycleEvents.h"
#include "enums/PortForwarding.h"
#include "enums/RouterState.h"
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
//NOTE: renewed at half, so the port stays open while the server runs; a killed one leaves it at most this long
constexpr auto kLease{20min};
//NOTE: the UPnP error of a router that takes permanent leases only; asked again with no lease time
constexpr int kOnlyPermanentLeases{725};
//NOTE: a port kept for good needs no renewal - asked again once a day all the same, in case the router forgot it
constexpr auto kPermanentRenewStep{24h};
//NOTE: a refused renewal is asked again this often - the lease still holds, and a restarted router takes it at once
constexpr auto kRefusedRenewStep{30s};
constexpr auto kInternetPollStep{50ms};
//NOTE: a router lists every mapping of every machine behind it - a long list is not walked to its end
constexpr int kMappingsRead{128};
//NOTE: a router found before answers in a fraction of a second, a search with nothing answering takes eight
constexpr auto kAnswerPatience{10s};

[[nodiscard]] bool IsEveryNetwork(const std::string& host)
{
	boost::system::error_code ec;
	const auto address{boost::asio::ip::make_address(host, ec)};

	return !ec && address.is_unspecified();
}

//NOTE: nothing here listens on it - Windows allows an every-network socket beside one on an address, so try both
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

//NOTE: drops our old servers' mappings to this machine on unused ports; read whole first, a removal renumbers
void CloseLeftovers(const network::Router& router, const std::string& client, const std::stop_token& stop)
{
	std::vector<std::string> leftovers{};
	for (int index{}; index < kMappingsRead && !stop.stop_requested(); ++index)
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
		if (stop.stop_requested())
		{
			return;
		}

		std::ignore = UPNP_DeletePortMapping(router.ControlUrl(), router.ServiceType(), port.c_str(), "UDP", nullptr);
		Log::Info("UPnP: UDP port " + port + ", left open on the router by an earlier server, is closed");
	}
}

//NOTE: a known one gone silent is searched for anew - a restarted router may tell its description on another port
[[nodiscard]] std::unique_ptr<const network::Router> FindRouter(const std::string& descriptionUrl)
{
	if (!descriptionUrl.empty())
	{
		auto known{std::make_unique<const network::Router>(descriptionUrl)};
		if (known->GetState() != RouterState::Silent)
		{
			return known;
		}

		Log::Info("UPnP: the router found before no longer answers at " + descriptionUrl + " - searched for anew");
	}

	return std::make_unique<const network::Router>();
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
PortMapping::PortMapping(std::string host, const std::uint16_t port, std::string routerUrl)
	: _host{std::move(host)}
	, _port{port}
	, _routerUrl{std::move(routerUrl)}
	, _answerDue{std::chrono::steady_clock::now() + kAnswerPatience}
	, _publicProbe{std::make_unique<PublicAddressProbe>()}
	, _change{PortForwardingChangedEvent{.state = PortForwarding::Opening, .port = port}}
	, _worker{[this](const std::stop_token& stop) { Forward(stop); }}
{
}

PortMapping::~PortMapping() = default;

std::optional<PortForwardingChangedEvent> PortMapping::Poll()
{
	const std::scoped_lock lock{_mutex};
	if (!_isAnswered && std::chrono::steady_clock::now() >= _answerDue)
	{
		_isAnswered = true;
		Log::Info("UPnP: no answer from the router in "
				  + std::to_string(std::chrono::seconds{kAnswerPatience}.count()) + "s - still asked");

		return PortForwardingChangedEvent{.state = PortForwarding::NoAnswer, .port = _port};
	}

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

	const auto found{FindRouter(_routerUrl)};
	const Router& router{*found};
	//NOTE: the search is over, whatever it found - the rest is quick or waits on the internet, not on the router
	{
		const std::scoped_lock lock{_mutex};
		_isAnswered = true;
	}

	if (router.GetState() == RouterState::Silent)
	{
		Log::Info("UPnP: no router answered - UDP port " + port + " has to be forwarded on it by hand");
		Report(PortForwarding::NoRouter);

		return;
	}

	if (router.GetState() == RouterState::Offline)
	{
		Log::Info("UPnP: nothing on the network is a router connected to the internet - UDP port " + port
				  + " has to be forwarded by hand");
		Report(PortForwarding::NoRouter);

		return;
	}

	if (router.GetState() == RouterState::BehindProvider)
	{
		Log::Info("UPnP: the router's outside address " + router.OutsideAddress()
				  + " is a provider's one - its NAT stands between the router and the internet, nobody gets in from "
					"there");
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

	CloseLeftovers(router, client, stop);
	if (stop.stop_requested())
	{
		return;
	}

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

	//NOTE: the internet's view is checked against the router's own address - behind another router it sees that one's
	const std::optional<std::string> seen{router.GetState() == RouterState::Online ? AskInternet(stop) : std::nullopt};
	const bool isBehindProvider{seen && *seen != router.OutsideAddress()};
	if (router.GetState() == RouterState::BehindRouter)
	{
		Log::Info("UPnP: UDP port " + port + " is open on the router, but another router stands in front of it - "
				  "the port has to be forwarded on that one to " + router.OutsideAddress() + " by hand");
	}
	else if (isBehindProvider)
	{
		Log::Info("UPnP: the router says it is " + router.OutsideAddress() + " on the internet, but the internet sees "
				  + *seen + " - a provider's NAT stands between, nobody gets in from there");
	}
	else
	{
		Log::Info("UPnP: UDP port " + port + " is open on the router, the internet reaches this server at "
				  + router.OutsideAddress() + ':' + port);
	}

	//NOTE: said again once a refused renewal goes through
	const auto reportOpen = [this, &router, isBehindProvider]
	{
		if (router.GetState() == RouterState::BehindRouter)
		{
			Report(PortForwarding::BehindRouter, router.OutsideAddress());
		}
		else if (isBehindProvider)
		{
			Report(PortForwarding::BehindProvider);
		}
		else
		{
			Report(PortForwarding::Open, router.OutsideAddress());
		}
	};
	reportOpen();

	const std::chrono::milliseconds renewStep{isPermanent ? kPermanentRenewStep : kLease / 2};
	bool isRenewalRefused{};
	while (Sleep(stop, isRenewalRefused ? kRefusedRenewStep : renewStep))
	{
		const int renewed{open()};
		if (renewed != UPNPCOMMAND_SUCCESS && !isRenewalRefused)
		{
			Log::Error("UPnP: the router did not renew UDP port " + port + " - " + ErrorText(renewed)
					   + ", asked again until it does");
			Report(PortForwarding::Lost);
		}
		else if (renewed == UPNPCOMMAND_SUCCESS && isRenewalRefused)
		{
			Log::Info("UPnP: the router forwards UDP port " + port + " again");
			reportOpen();
		}

		isRenewalRefused = renewed != UPNPCOMMAND_SUCCESS;
	}

	std::ignore = UPNP_DeletePortMapping(router.ControlUrl(), router.ServiceType(), port.c_str(), "UDP", nullptr);
	Log::Info("UPnP: UDP port " + port + " is closed on the router again");
}
}//namespace network
