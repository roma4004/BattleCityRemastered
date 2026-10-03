#include "network/ServerNode.h"
#include "network/Server.h"
#include "network/Endpoints.h"
#include "components/MatchSettings.h"
#include <boost/asio/post.hpp>
#include <functional>
#include <memory>

namespace network::commands
{
ServerNode::ServerNode(const ServerAddress& address, const std::shared_ptr<EventSystem>& events,
					   const MatchSettings& match)
	: NetworkNodeBase(events, "ServerNode")
	, _server{std::make_unique<Server>(IoContext(), address, events, match)}
{
	StartIoThread();
	SubscribeToNetCommandUpdate();
}

ServerNode::~ServerNode()
{
	StopIoThread([this](const std::function<void()>& done)
	{
		_server->Shutdown(DisconnectReason::HostShutdown, done);
	});
}

void ServerNode::ProcessNetworkCommands() { _server->ProcessNetworkCommands(); }

uint16_t ServerNode::GetBoundPort() const noexcept { return _server->GetBoundPort(); }

void ServerNode::Abort()
{
	boost::asio::post(IoContext(), [server = _server.get()] { server->Shutdown(); });
}
}//namespace network::commands
