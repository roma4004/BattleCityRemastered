#pragma once

#include "Server.h"
#include "components/EventSystem.h"
#include "interfaces/INetworkNode.h"
#include <thread>
#include <vector>

struct NetCommandUpdateEvent;

namespace network::commands
{
class ServerHandler : public INetworkNode
{
public:
	explicit ServerHandler(const std::shared_ptr<EventSystem>& events);
	ServerHandler(std::string host, uint16_t port, const std::shared_ptr<EventSystem>& events);

	~ServerHandler() override;

	void ProcessNetworkCommands() override
	{
		_server.ProcessNetworkCommands();
	}

	[[nodiscard]] uint16_t GetBoundPort() const { return _server.GetBoundPort(); }

private:
	void Subscribe();
	void Shutdown();
	void OnNetCommandUpdate(const NetCommandUpdateEvent&);

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	boost::asio::io_context _ioContext{};
	std::string _name{};
	std::thread _serverThread{};
	Server _server;
};

}//namespace network::commands
