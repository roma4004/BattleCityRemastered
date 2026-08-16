#pragma once

#include "NetworkNodeBase.h"
#include "Server.h"

namespace network::commands
{
class ServerHandler final : public NetworkNodeBase
{
public:
	explicit ServerHandler(const std::shared_ptr<EventSystem>& events);
	ServerHandler(std::string host, uint16_t port, const std::shared_ptr<EventSystem>& events);

	~ServerHandler() override;

	void ProcessNetworkCommands() override { _server.ProcessNetworkCommands(); }

	[[nodiscard]] uint16_t GetBoundPort() const { return _server.GetBoundPort(); }

private:
	Server _server;
};

}//namespace network::commands
