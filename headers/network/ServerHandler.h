#pragma once

#include "Server.h"
#include "interfaces/INetworkNode.h"
#include <thread>

namespace network::commands
{
class ServerHandler : public INetworkNode
{
	std::shared_ptr<EventSystem> _events{nullptr};
	boost::asio::io_context _ioContext{};
	Server _server;
	std::thread _serverThread{};
	std::string _name{};

public:
	explicit ServerHandler(const std::shared_ptr<EventSystem>& events);
	ServerHandler(const std::string& host, const std::string& port, const std::shared_ptr<EventSystem>& events);

	~ServerHandler() override;

	void Subscribe();
	void Unsubscribe() const;

	void ProcessNetworkCommands() override
	{
		_server.ProcessNetworkCommands();
	}
};
}//namespace network::commands
