#pragma once

#include <cstdint>
#include <memory>
#include <optional>

//NOTE: the handle to BattleCityServer, no network code of its own. The job object kills the child
//when the game is killed rather than closed - a survivor would hold the port
namespace network
{
struct ServerAddress;
}//namespace network

class ServerProcess final
{
	struct Process;

	//NOTE: pimpl to keep windows.h out of every translation unit that includes this
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
	[[nodiscard]] bool Start(const network::ServerAddress& address);

	//NOTE: the port a server started next to us wrote down, whoever started it - the only number a
	//client has to dial while the lobby cannot ask for one. Empty until a server publishes it
	[[nodiscard]] static std::optional<std::uint16_t> PublishedPort();

	[[nodiscard]] bool IsRunning() const;
};
