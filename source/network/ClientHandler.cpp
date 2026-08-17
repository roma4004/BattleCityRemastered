#include "network/ClientHandler.h"

namespace network::commands
{
ClientHandler::ClientHandler(const std::shared_ptr<EventSystem>& events)
	: ClientHandler(std::string("127.0.0.1"), 1234u, events) {}

ClientHandler::ClientHandler(std::string host, uint16_t port, const std::shared_ptr<EventSystem>& events)
	: NetworkNodeBase(events, "ClientHandler")
	, _client{std::make_shared<Client>(IoContext(), std::move(host), port, events)}
{
	StartIoThread();
	SubscribeToNetCommandUpdate();
}

ClientHandler::~ClientHandler()
{
	//NOTE: only the local player leaves through here - a host-side teardown arrives as a command
	StopIoThread([client = _client](std::function<void()> done)
	{
		client->Shutdown(DisconnectReason::PlayerQuit, std::move(done));
	});
}
}//namespace network::commands
