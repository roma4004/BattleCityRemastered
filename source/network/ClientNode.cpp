#include "network/ClientNode.h"
#include "network/Client.h"
#include <boost/asio/post.hpp>

namespace network
{
ClientNode::ClientNode(const ServerAddress& address, const std::shared_ptr<EventSystem>& events)
	: NetworkNodeBase(events, "ClientNode")
	, _client{std::make_shared<Client>(IoContext(), address, events)}
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

void ClientNode::ProcessNetworkCommands() { _client->ProcessCommandQueue(); }

void ClientNode::Abort()
{
	boost::asio::post(IoContext(), [client = _client] { client->Shutdown(); });
}
}//namespace network
