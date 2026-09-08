#include "network/ServerNode.h"
#include "network/Endpoints.h"
#include "network/Server.h"
#include <boost/asio/post.hpp>

namespace network::commands
{
ServerNode::ServerNode(const std::shared_ptr<EventSystem>& events)
	: ServerNode(std::string(kDefaultHost), kDefaultPort, events) {}

ServerNode::ServerNode(std::string host, uint16_t port, const std::shared_ptr<EventSystem>& events)
	: NetworkNodeBase(events, "ServerNode")
	, _server{std::make_unique<Server>(IoContext(), std::move(host), port, events)}
{
	StartIoThread();
	SubscribeToNetCommandUpdate();
}

ServerNode::~ServerNode()
{
	StopIoThread([this](std::function<void()> done)
	{
		_server->Shutdown(DisconnectReason::HostShutdown, done);
	});
}

void ServerNode::ProcessNetworkCommands() { _server->ProcessNetworkCommands(); }

uint16_t ServerNode::GetBoundPort() const { return _server->GetBoundPort(); }

void ServerNode::Abort()
{
	boost::asio::post(IoContext(), [server = _server.get()] { server->Shutdown(); });
}
}//namespace network::commands
