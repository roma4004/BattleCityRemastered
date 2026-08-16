#pragma once

#include "Client.h"
#include "NetworkNodeBase.h"

namespace network::commands
{
class ClientHandler final : public NetworkNodeBase
{
public:
	explicit ClientHandler(const std::shared_ptr<EventSystem>& events);
	ClientHandler(std::string host, uint16_t port, const std::shared_ptr<EventSystem>& events);

	~ClientHandler() override;

	void ProcessNetworkCommands() override
	{
		if (_client)
		{
			_client->ProcessCommandQueue();
		}
	}

	[[nodiscard]] bool IsConnected() const { return _client && _client->IsConnected(); }

private:
	std::shared_ptr<Client> _client{nullptr};
};

}//namespace network::commands
