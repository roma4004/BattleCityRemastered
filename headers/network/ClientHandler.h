#pragma once

#include "Client.h"
#include "../interfaces/INetworkNode.h"

#include <thread>

class ClientHandler : public INetworkNode
{
	std::shared_ptr<EventSystem> _events{nullptr};
	boost::asio::io_context _ioContext;
	boost::asio::ip::tcp::socket _socket;
	Client _client;
	std::thread _clientThread;

public:
	explicit ClientHandler(std::shared_ptr<EventSystem> events);

	~ClientHandler();
};
