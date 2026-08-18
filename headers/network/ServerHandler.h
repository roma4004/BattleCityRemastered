#pragma once

#include "NetworkNodeBase.h"
#include <cstdint>
#include <memory>
#include <string>

namespace network::commands
{
class Server;

class ServerHandler final : public NetworkNodeBase
{
public:
	explicit ServerHandler(const std::shared_ptr<EventSystem>& events);
	ServerHandler(std::string host, uint16_t port, const std::shared_ptr<EventSystem>& events);

	~ServerHandler() override;

	void ProcessNetworkCommands() override;

	[[nodiscard]] uint16_t GetBoundPort() const;

	//NOTE: no goodbye, the way a crashed host would go - the peer sees a bare EOF and reconnects,
	//unlike the destructor's announced leave
	void Abort();

private:
	//NOTE: by pointer only to keep Server - and with it asio - out of this header
	std::unique_ptr<Server> _server;
};

}//namespace network::commands
