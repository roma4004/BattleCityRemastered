#pragma once

#include "NetworkNodeBase.h"
#include <cstdint>
#include <memory>
#include <string>

namespace network::commands
{
class Client;

class ClientHandler final : public NetworkNodeBase
{
public:
	explicit ClientHandler(const std::shared_ptr<EventSystem>& events);
	ClientHandler(std::string host, uint16_t port, const std::shared_ptr<EventSystem>& events);

	~ClientHandler() override;

	void ProcessNetworkCommands() override;

	[[nodiscard]] bool IsConnected() const;

	void Abort();

private:
	//NOTE: Client stays incomplete here - it is what drags asio in, and only the .cpp needs it
	std::shared_ptr<Client> _client{nullptr};
};

}//namespace network::commands
