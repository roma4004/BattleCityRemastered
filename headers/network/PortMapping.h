#pragma once

#include "components/events/CoreLifecycleEvents.h"
#include "enums/PortForwarding.h"
#include <chrono>
#include <condition_variable>
#include <cstdint>
#include <memory>
#include <mutex>
#include <optional>
#include <stop_token>
#include <string>
#include <thread>

namespace network
{
class PublicAddressProbe;

//NOTE: holds the server port open on the router via UPnP from a thread; a killed process leaves it till the lease ends
class PortMapping final
{
public:
	//NOTE: a server on every network is forwarded to this machine's address; an empty routerUrl means a search
	PortMapping(std::string host, std::uint16_t port, std::string routerUrl);

	~PortMapping();

	PortMapping(const PortMapping&) = delete;
	PortMapping& operator=(const PortMapping&) = delete;
	PortMapping(PortMapping&&) = delete;
	PortMapping& operator=(PortMapping&&) = delete;

	//NOTE: what changed since the last call - Opening comes first, at once
	[[nodiscard]] std::optional<PortForwardingChangedEvent> Poll();

private:
	void Forward(const std::stop_token& stop);
	void Report(PortForwarding state, std::string host = {});
	//NOTE: what the internet sees, so a router behind another NAT is told from one that is not
	[[nodiscard]] std::optional<std::string> AskInternet(const std::stop_token& stop);
	//NOTE: false when stopped instead
	[[nodiscard]] bool Sleep(const std::stop_token& stop, std::chrono::milliseconds duration);

	const std::string _host;
	const std::uint16_t _port;
	const std::string _routerUrl;
	//NOTE: past it with no word from the router, the host is told it does not answer
	const std::chrono::steady_clock::time_point _answerDue;
	//NOTE: built at once so the STUN names resolve while the router is searched; only the worker touches it afterwards
	std::unique_ptr<PublicAddressProbe> _publicProbe;
	std::mutex _mutex;
	std::condition_variable_any _wake;
	std::optional<PortForwardingChangedEvent> _change{};
	bool _isAnswered{};
	//NOTE: last, so it is joined before anything it touches goes
	std::jthread _worker{};
};
}//namespace network
