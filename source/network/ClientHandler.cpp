#include "network/ClientHandler.h"
#include "components/EventSystem.h"
#include <boost/asio/io_context.hpp>
#include <boost/asio/post.hpp>
#include <chrono>
#include <future>

namespace network::commands
{
ClientHandler::ClientHandler(const std::shared_ptr<EventSystem>& events)
	: ClientHandler(std::string("127.0.0.1"), 1234u, events) {}

ClientHandler::ClientHandler(std::string host, uint16_t port, const std::shared_ptr<EventSystem>& events)
	: _events{events}
	, _name{"ClientHandler"}
	, _client{std::make_shared<Client>(_ioContext, host, port, events)}
{
	_clientThread = std::thread([this]()
	{
		try
		{
			this->_ioContext.run();
			// io_service.stop();
		}
		catch (std::exception& e)
		{
			std::cerr << "ClientHandler thread " << e.what() << '\n';
			//TODO: write error to file
		}
		catch (...)
		{
			std::cerr << "ClientHandler thread error ..." << '\n';
			//TODO: write error to file
		}
	});

	Subscribe();
}

ClientHandler::~ClientHandler()
{
	Shutdown();
	Unsubscribe();
}

void ClientHandler::Shutdown()
{
	if (_client && !_ioContext.stopped())
	{
		auto shutdownDone{std::make_shared<std::promise<void>>()};
		const std::future<void> shutdownFuture{shutdownDone->get_future()};

		boost::asio::post(_ioContext, [client = _client, shutdownDone]()
		{
			client->Shutdown();
			shutdownDone->set_value();
		});

		shutdownFuture.wait_for(std::chrono::milliseconds(500));
	}

	if (!_ioContext.stopped())
	{
		_ioContext.stop();
	}

	if (_clientThread.joinable())
	{
		_clientThread.join();
	}
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
