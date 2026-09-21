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
	std::optional<std::uint16_t> _boundPort{};

public:
	ServerProcess();

	~ServerProcess();

	ServerProcess(const ServerProcess&) = delete;
	ServerProcess(ServerProcess&&) = delete;
	ServerProcess& operator=(const ServerProcess&) = delete;
	ServerProcess& operator=(ServerProcess&&) = delete;

	//NOTE: looks for the exe next to our own, never on PATH. Gives back the port the child is listening
	//on - the same one it was asked for, unless that was 0 and the OS picked. Empty means it never came up
	[[nodiscard]] std::optional<std::uint16_t> Start(const network::ServerAddress& address);

	//NOTE: the port a server started next to us wrote down, whoever started it - the only number a
	//client has to dial while the lobby cannot ask for one. Empty until a server publishes it
	[[nodiscard]] static std::optional<std::uint16_t> PublishedPort();

	[[nodiscard]] bool IsRunning() const;
};
