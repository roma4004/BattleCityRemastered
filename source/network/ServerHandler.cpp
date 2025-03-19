#include "../../headers/network/ServerHandler.h"

#include <iostream>
#include <boost/asio/io_context.hpp>

ServerHandler::ServerHandler(std::shared_ptr<EventSystem> events)
	: _events{std::move(events)},
	  _server{_ioContext, "127.0.0.1", "1234", _events}
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
	_ioContext.stop();
	_serverThread.join();
};
