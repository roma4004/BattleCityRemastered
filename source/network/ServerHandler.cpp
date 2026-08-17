#include "network/ServerHandler.h"

namespace network::commands
{
ServerHandler::ServerHandler(const std::shared_ptr<EventSystem>& events)
	: ServerHandler(std::string("127.0.0.1"), 1234u, events) {}

ServerHandler::ServerHandler(std::string host, uint16_t port, const std::shared_ptr<EventSystem>& events)
	: NetworkNodeBase(events, "ServerHandler")
	, _server{IoContext(), std::move(host), port, events}
{
	StartIoThread();
	SubscribeToNetCommandUpdate();
}

ServerHandler::~ServerHandler()
{
	StopIoThread([this](std::function<void()> done)
	{
		_server.Shutdown(DisconnectReason::HostShutdown, std::move(done));
	});
}
}//namespace network::commands
