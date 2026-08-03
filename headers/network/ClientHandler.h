#pragma once

#include "Client.h"
#include "interfaces/INetworkNode.h"
#include <thread>

namespace network::commands
{
class ClientHandler : public INetworkNode
{
public:
	explicit ClientHandler(const std::shared_ptr<EventSystem>& events);
	ClientHandler(std::string host, uint16_t port, const std::shared_ptr<EventSystem>& events);

	~ClientHandler() override;

	void ProcessNetworkCommands() override
	{
		if (_client)
		{
			_client->GetCommandQueue().ProcessAll();//TODO: refactor to _client->ProcessCommandQueue()
		}
	}

	[[nodiscard]] bool IsConnected() const { return _client && _client->IsConnected(); }

private:
	void Subscribe();
	void Unsubscribe() const;
	void Shutdown();

	std::shared_ptr<EventSystem> _events{nullptr};
	boost::asio::io_context _ioContext{};
	// boost::asio::ip::tcp::socket _socket;
	std::string _name{};
	std::thread _clientThread{};
	std::shared_ptr<Client> _client{nullptr};
};

}//namespace network::commands
