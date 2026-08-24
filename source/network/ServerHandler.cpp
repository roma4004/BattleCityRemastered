#include "network/ServerHandler.h"
#include "network/Server.h"
#include <boost/asio/post.hpp>

namespace network::commands
{
ServerHandler::ServerHandler(const std::shared_ptr<EventSystem>& events)
	: ServerHandler(std::string("127.0.0.1"), 1234u, events) {}

ServerHandler::ServerHandler(std::string host, uint16_t port, const std::shared_ptr<EventSystem>& events)
	: NetworkNodeBase(events, "ServerHandler")
	, _server{std::make_unique<Server>(IoContext(), std::move(host), port, events)}
{
	StartIoThread();
	SubscribeToNetCommandUpdate();
}

ServerHandler::~ServerHandler()
{
	StopIoThread([this](std::function<void()> done)
	{
		_server->Shutdown(DisconnectReason::HostShutdown, std::move(done));
	});
}

void ServerHandler::ProcessNetworkCommands() { _server->ProcessNetworkCommands(); }

uint16_t ServerHandler::GetBoundPort() const { return _server->GetBoundPort(); }

void ServerHandler::Abort()
{
	boost::asio::post(IoContext(), [server = _server.get()] { server->Shutdown(); });
}
}//namespace network::commands
