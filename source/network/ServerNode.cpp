#include "network/ServerNode.h"
#include "network/Server.h"
#include "network/Endpoints.h"
#include "components/MatchSettings.h"
#include <functional>
#include <memory>
#include <optional>

namespace network
{
ServerNode::ServerNode(const ServerAddress& address, const std::shared_ptr<EventSystem>& events,
					   const MatchSettings& match, const std::optional<ServerAddress>& catalog)
	: NetworkNodeBase(events, "ServerNode")
	, _server{std::make_unique<Server>(IoContext(), address, events, match, catalog)}
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
}//namespace network
