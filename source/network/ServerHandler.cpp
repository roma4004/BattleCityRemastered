#include "network/ServerHandler.h"
#include <boost/asio/io_context.hpp>
#include <iostream>

namespace network::commands
{
ServerHandler::ServerHandler(const std::shared_ptr<EventSystem>& events)
	: _events{events}
	, _server{_ioContext, "127.0.0.1", "1234", _events}
{
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
};
}//namespace network::commands
