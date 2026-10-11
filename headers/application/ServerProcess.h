#pragma once

#include <cstdint>
#include <memory>
#include <optional>
#include <string>

//NOTE: no network code of its own. A game killed rather than closed takes the server with it, or it holds the port
struct MatchSettings;

namespace network
{
struct ServerAddress;
}//namespace network

class ServerProcess final
{
	struct Process;

	//NOTE: pimpl to keep windows.h and the POSIX headers out of every translation unit that includes this
	std::unique_ptr<Process> _process;

public:
	ServerProcess();

	~ServerProcess();

	ServerProcess(const ServerProcess&) = delete;
	ServerProcess(ServerProcess&&) = delete;
	ServerProcess& operator=(const ServerProcess&) = delete;
	ServerProcess& operator=(ServerProcess&&) = delete;

	//NOTE: looks for the exe next to our own, never on PATH. Does not wait for it to listen - the port turns
	//up in PublishedPort once it does
	[[nodiscard]] bool Start(const network::ServerAddress& address, const MatchSettings& match, bool isPortForwarded,
							 const std::string& routerUrl);

	//NOTE: the port a server started next to us wrote down, whoever started it - the only number a
	//client has to dial while the lobby cannot ask for one. Empty until a server publishes it
	[[nodiscard]] static std::optional<std::uint16_t> PublishedPort();

	[[nodiscard]] bool IsRunning() const;

	[[nodiscard]] bool CouldNotListen() const;
};
