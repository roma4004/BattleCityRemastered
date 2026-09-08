#pragma once

#include <memory>

//NOTE: the handle to BattleCityServer, no network code of its own. The job object kills the child
//when the game is killed rather than closed - a survivor would hold the port
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

	//NOTE: looks for the exe next to our own, never on PATH. True for a server that is already up
	[[nodiscard]] bool Start();

	[[nodiscard]] bool IsRunning() const;
};
