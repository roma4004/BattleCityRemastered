#include "network/ServerHandler.h"
#include "components/EventSystem.h"
#include "components/events/TimingEvents.h"
#include "utils/NetworkLogger.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <chrono>
#include <future>
#include <iostream>

namespace network::commands
{
ServerHandler::ServerHandler(const std::shared_ptr<EventSystem>& events)
	: ServerHandler(std::string("127.0.0.1"), 1234u, events) {}

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
			NetworkLogger::WriteLog(std::string("ServerHandler thread exception: ") + e.what());
		}
		catch (...)
		{
			std::cerr << "ServerHandler thread error ..." << '\n';
			NetworkLogger::WriteLog("ServerHandler thread error: unknown exception");
		}
	});

	Subscribe();
}

ServerHandler::~ServerHandler()
{
	Shutdown();
}

void ServerHandler::Shutdown()
{
	if (!_ioContext.stopped())
	{
		auto shutdownDone{std::make_shared<std::promise<void>>()};
		const std::future<void> shutdownFuture{shutdownDone->get_future()};

		boost::asio::post(_ioContext, [this, shutdownDone]()
		{
			_server.Shutdown();
			shutdownDone->set_value();
		});

		shutdownFuture.wait_for(std::chrono::milliseconds(500));
	}

	if (!_ioContext.stopped())
	{
		_ioContext.stop();
	}

	if (_serverThread.joinable())
	{
		_serverThread.join();
	}
}

void ServerHandler::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &ServerHandler::OnNetCommandUpdate));
}

void ServerHandler::OnNetCommandUpdate(const NetCommandUpdateEvent&) { ProcessNetworkCommands(); }

}//namespace network::commands
