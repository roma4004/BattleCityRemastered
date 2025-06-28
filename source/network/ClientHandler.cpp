#include "network/ClientHandler.h"
#include <boost/asio/io_context.hpp>

ClientHandler::ClientHandler(std::shared_ptr<EventSystem> events)
	: _events{std::move(events)},
	  // _socket(_ioContext),
	  _client{_ioContext, "127.0.0.1", "1234", _events}
{
	// socket.connect(boost::asio::ip::tcp::endpoint(boost::asio::ip::address::from_string("127.0.0.1"), 1234));
	_clientThread = std::thread([&]() { _ioContext.run(); });
}

ClientHandler::ClientHandler(const std::string& host, const std::string& port, std::shared_ptr<EventSystem> events)
	: _events{std::move(events)},
	  // _socket(_ioContext),
	  _client{_ioContext, host, port, _events}
{
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
};
