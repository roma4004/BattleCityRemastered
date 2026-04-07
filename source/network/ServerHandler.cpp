#include "network/ServerHandler.h"
#include "components/EventSystem.h"
#include <boost/asio/io_context.hpp>
#include <iostream>

namespace network::commands
{
ServerHandler::ServerHandler(const std::shared_ptr<EventSystem>& events)
	: _events{events}
	, _server{_ioContext, "127.0.0.1", "1234", _events}
{
	_name = "ServerHandler";
	_events->AddListener("NetCommandUpdate", _name, [this](const double /*deltaTime*/)
	{
		this->ProcessNetworkCommands();
	});

	_serverThread = std::thread([&]()
	{
		try
		{
			_ioContext.run();
			// io_service.stop();
		}
		catch (std::exception& e)
		{
			std::cerr << "thread " << e.what() << '\n';
		}
		catch (...)
		{
			std::cerr << "thread error ..." << '\n';
		}
	});
}

ServerHandler::ServerHandler(const std::string& host, const std::string& port,
							 const std::shared_ptr<EventSystem>& events)
	: _events{events}
	, _server{_ioContext, host, port, _events}
{
	_name = "ServerHandler";
	_events->AddListener("NetCommandUpdate", _name, [this](const double /*deltaTime*/)
	{
		this->ProcessNetworkCommands();//TODO: move to separated method subscribe/unsubscribe, and same for client
	});

	_serverThread = std::thread([&]()
	{
		try
		{
			_ioContext.run();
			// io_service.stop();
		}
		catch (std::exception& e)
		{
			std::cerr << "thread " << e.what() << '\n';
		}
		catch (...)
		{
			std::cerr << "thread error ..." << '\n';
		}
	});
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

	_events->RemoveListener("NetCommandUpdate", _name);
}
}//namespace network::commands
