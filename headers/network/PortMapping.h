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

//NOTE: the server's UDP port opened on the router through UPnP for as long as this lives - renewed before its
//lease runs out and closed on the way out. A router takes seconds to find, so the work is done on a thread of
//its own and the server's loop polls for the outcome. A process killed outright leaves the port open until
//the lease runs out on the router
class PortMapping final
{
public:
	//NOTE: a server listening on every network is forwarded to this machine's address on the router's one
	PortMapping(std::string host, std::uint16_t port);

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
	//NOTE: built at once, so the servers' names resolve while the router is looked for. Only the thread touches it
	//afterwards
	std::unique_ptr<PublicAddressProbe> _publicProbe;
	std::mutex _mutex;
	std::condition_variable_any _wake;
	std::optional<PortForwardingChangedEvent> _change{};
	//NOTE: last, so it is joined before anything it touches goes
	std::jthread _worker{};
};
}//namespace network
