#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

struct MatchSettings;

namespace network
{
struct ServerAddress;
}//namespace network

//NOTE: no network code; a killed game takes the server down so the port is freed
class ServerProcess final
{
	struct Process;

	std::unique_ptr<Process> _process;

public:
	ServerProcess();

	~ServerProcess();

	ServerProcess(const ServerProcess&) = delete;
	ServerProcess(ServerProcess&&) = delete;
	ServerProcess& operator=(const ServerProcess&) = delete;
	ServerProcess& operator=(ServerProcess&&) = delete;

	//NOTE: runs the exe next to ours, not from PATH; returns at once - PublishedPort has the port once it listens
	[[nodiscard]] bool Start(const network::ServerAddress& address, const MatchSettings& match, bool isPortForwarded,
							 const std::string& routerUrl);

	//NOTE: the port written by a server running next to us, whoever started it
	[[nodiscard]] static std::optional<std::uint16_t> PublishedPort();

	[[nodiscard]] bool IsRunning() const;

	[[nodiscard]] bool CouldNotListen() const;
};
