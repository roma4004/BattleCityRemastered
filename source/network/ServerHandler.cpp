#include "network/ServerHandler.h"
#include "components/EventSystem.h"
#include <boost/asio/io_context.hpp>
#include <iostream>

namespace network::commands
{
ServerHandler::ServerHandler(const std::shared_ptr<EventSystem>& events)
	: ServerHandler(std::string("127.0.0.1"), 1234, events) {}

ServerHandler::ServerHandler(std::string host, uint16_t port, const std::shared_ptr<EventSystem>& events)
	: _events{events}
	, _name{"ServerHandler"}
	, _server{_ioContext, host, port, events}
{
	_serverThread = std::thread([this]()
	{
		try
		{
			this->_ioContext.run();
			// this->io_service.stop();
		}
		catch (std::exception& e)
		{
			std::cerr << "ServerHandler thread " << e.what() << '\n';
			//TODO: write error to file
		}
		catch (...)
		{
			std::cerr << "ServerHandler thread error ..." << '\n';
			//TODO: write error to file
		}
	});

	Subscribe();
}

ServerHandler::~ServerHandler()
{
	if (!_ioContext.stopped())
	{
		_ioContext.stop();
	}

	if (_serverThread.joinable())
	{
		_serverThread.join();
	}

	Unsubscribe();
}

void ServerHandler::Subscribe()
{
	_events->AddListener("NetCommandUpdate", _name, [this](const double /*deltaTime*/)
	{
		this->ProcessNetworkCommands();
	});
}

void ServerHandler::Unsubscribe() const
{
	_events->RemoveListener("NetCommandUpdate", _name);
}

}//namespace network::commands
