#include "network/ClientNode.h"
#include "network/Client.h"
#include "network/Endpoints.h"
#include <boost/asio/post.hpp>

namespace network::commands
{
ClientNode::ClientNode(const std::shared_ptr<EventSystem>& events)
	: ClientNode(std::string(kDefaultHost), kDefaultPort, events) {}

ClientNode::ClientNode(std::string host, uint16_t port, const std::shared_ptr<EventSystem>& events)
	: NetworkNodeBase(events, "ClientNode")
	, _client{std::make_shared<Client>(IoContext(), std::move(host), port, events)}
{
	StartIoThread();
	SubscribeToNetCommandUpdate();
}

ClientNode::~ClientNode()
{
	//NOTE: only the local player leaves through here - a host-side teardown arrives as a command
	StopIoThread([client = _client](std::function<void()> done)
	{
		client->Shutdown(DisconnectReason::PlayerQuit, std::move(done));
	});
}

void ClientNode::ProcessNetworkCommands()
{
	if (_client)
	{
		_client->ProcessCommandQueue();
	}
}

bool ClientNode::IsConnected() const { return _client && _client->IsConnected(); }

void ClientNode::Abort()
{
	boost::asio::post(IoContext(), [client = _client] { client->Shutdown(); });
}
}//namespace network::commands
