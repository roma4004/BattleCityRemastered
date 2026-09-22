#pragma once

#include "Endpoints.h"
#include "NetworkNodeBase.h"
#include <memory>

namespace network::commands
{
class Client;

class ClientNode final : public NetworkNodeBase
{
public:
	ClientNode(const ServerAddress& address, const std::shared_ptr<EventSystem>& events);

	~ClientNode() override;

	void ProcessNetworkCommands() override;

	[[nodiscard]] bool IsConnected() const noexcept;

	void Abort();

private:
	//NOTE: Client stays incomplete here - it is what drags asio in, and only the .cpp needs it
	std::shared_ptr<Client> _client{nullptr};
};

}//namespace network::commands
