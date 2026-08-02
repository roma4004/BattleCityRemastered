#pragma once

#include "Server.h"
#include "interfaces/INetworkNode.h"
#include <thread>

namespace network::commands
{
class ServerHandler : public INetworkNode
{
public:
	explicit ServerHandler(const std::shared_ptr<EventSystem>& events);
	ServerHandler(std::string host, uint16_t port, const std::shared_ptr<EventSystem>& events);

	~ServerHandler() override;

	void ProcessNetworkCommands() override
	{
		_server.ProcessNetworkCommands();
	}

private:
	void Subscribe();
	void Unsubscribe() const;
	void Shutdown();

	std::shared_ptr<EventSystem> _events{nullptr};
	boost::asio::io_context _ioContext{};
	std::string _name{};
	std::thread _serverThread{};
	Server _server;
};

}//namespace network::commands
