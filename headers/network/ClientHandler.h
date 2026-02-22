#pragma once

#include "Client.h"
#include "interfaces/INetworkNode.h"
#include <thread>

namespace network::commands
{
class ClientHandler : public INetworkNode
{
	std::shared_ptr<EventSystem> _events{nullptr};
	boost::asio::io_context _ioContext{};
	// boost::asio::ip::tcp::socket _socket;
	Client _client;
	std::thread _clientThread{};

public:
	explicit ClientHandler(const std::shared_ptr<EventSystem>& events);
	ClientHandler(const std::string& host, const std::string& port, const std::shared_ptr<EventSystem>& events);

	~ClientHandler();
};
}//namespace network::commands
