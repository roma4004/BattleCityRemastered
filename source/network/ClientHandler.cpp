#include "network/ClientHandler.h"
#include "components/EventSystem.h"
#include <boost/asio/io_context.hpp>

namespace network::commands
{
ClientHandler::ClientHandler(const std::shared_ptr<EventSystem>& events)
	: ClientHandler("127.0.0.1", "1234", events) {}

ClientHandler::ClientHandler(const std::string& host, const std::string& port,
							 const std::shared_ptr<EventSystem>& events)
	: _events{events}
	,// _socket(_ioContext),
	_client{std::make_shared<Client>(_ioContext, host, port, _events)}
{
	_name = "ClientHandler";
	Subscribe();

	// socket.connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 1234));
	_clientThread = std::thread([&]() { _ioContext.run(); });
}

ClientHandler::~ClientHandler()
{
	if (!_ioContext.stopped())
	{
		_ioContext.stop();
	}

	if (_clientThread.joinable())
	{
		_clientThread.join();
	}

	Unsubscribe();
}

void ClientHandler::Subscribe()
{
	_events->AddListener("NetCommandUpdate", _name, [this](const double /*deltaTime*/)
	{
		this->ProcessNetworkCommands();
	});
}

void ClientHandler::Unsubscribe() const
{
	_events->RemoveListener("NetCommandUpdate", _name);
}

}//namespace network::commands
