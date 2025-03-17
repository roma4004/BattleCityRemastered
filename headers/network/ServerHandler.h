#pragma once

#include "Server.h"
#include "../EventSystem.h"
#include "../interfaces/INetworkNode.h"

#include <thread>

class ServerHandler : public INetworkNode
{
	std::shared_ptr<EventSystem> _events{nullptr};
	boost::asio::io_context _ioContext;
	Server _server;
	std::thread _serverThread;

public:
	explicit ServerHandler(std::shared_ptr<EventSystem> events);

	~ServerHandler();
};
