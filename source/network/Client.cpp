#include "../../headers/network/Client.h"
#include "../../headers/components/EventSystem.h"

#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

// std::ofstream error_log("error_log_client.txt");

Client::Client(boost::asio::io_context& ioContext, const std::string& host, const std::string& port,
               std::shared_ptr<EventSystem> events)
	: _socket(ioContext),
	  _events{std::move(events)},
	  _name{"Client"}
{
	Subscribe();

	tcp::resolver resolver(ioContext);
	const auto endpointIterator = resolver.resolve(host, port);
	boost::asio::async_connect(
			_socket, endpointIterator,
			[this](const boost::system::error_code& ec, tcp::endpoint /*endpoint_iterator*/)
			{
				if (!ec)
				{
					// Init connection success, start reading from socket
					this->ReadResponse();
				}
			});
}

Client::~Client()
{
	Unsubscribe();

	try
	{
		if (_socket.is_open())
		{
			boost::system::error_code ec;
			_socket.shutdown(tcp::socket::shutdown_both, ec);
			if (ec)
			{
				std::cerr << "Error during socket shutdown: " << ec.message() << std::endl;
			}

			_socket.close(ec);
			if (ec)
			{
				std::cerr << "Error during socket close: " << ec.message() << std::endl;
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception in ~Client: " << e.what() << '\n';
	}
	catch (...)
	{
		std::cerr << "Unknown error in ~Client" << '\n';
	}
}

void Client::Subscribe()
{
	_events->AddListener("ArrowUp_Pressed", _name, [this]() { this->SendKeyState("ArrowUp_Pressed"); });
	_events->AddListener("ArrowLeft_Pressed", _name, [this]() { this->SendKeyState("ArrowLeft_Pressed"); });
	_events->AddListener("ArrowDown_Pressed", _name, [this]() { this->SendKeyState("ArrowDown_Pressed"); });
	_events->AddListener("ArrowRight_Pressed", _name, [this]() { this->SendKeyState("ArrowRight_Pressed"); });
	_events->AddListener("RCTRL_Pressed", _name, [this]() { this->SendKeyState("RCTRL_Pressed"); });

	_events->AddListener("ArrowUp_Released", _name, [this]() { this->SendKeyState("ArrowUp_Released"); });
	_events->AddListener("ArrowLeft_Released", _name, [this]() { this->SendKeyState("ArrowLeft_Released"); });
	_events->AddListener("ArrowDown_Released", _name, [this]() { this->SendKeyState("ArrowDown_Released"); });
	_events->AddListener("ArrowRight_Released", _name, [this]() { this->SendKeyState("ArrowRight_Released"); });
	_events->AddListener("RCTRL_Released", _name, [this]() { this->SendKeyState("RCTRL_Released"); });
}

void Client::Unsubscribe() const
{
	_events->RemoveListener("ArrowUp_Pressed", _name);
	_events->RemoveListener("ArrowLeft_Pressed", _name);
	_events->RemoveListener("ArrowDown_Pressed", _name);
	_events->RemoveListener("ArrowRight_Pressed", _name);
	_events->RemoveListener("RCTRL_Pressed", _name);

	_events->RemoveListener("ArrowUp_Released", _name);
	_events->RemoveListener("ArrowLeft_Released", _name);
	_events->RemoveListener("ArrowDown_Released", _name);
	_events->RemoveListener("ArrowRight_Released", _name);
	_events->RemoveListener("RCTRL_Released", _name);
}

void Client::ReadResponse()
{
	// auto self(shared_from_this());
	auto lambda = [this, events = _events](const boost::system::error_code& ec, const std::size_t length)
	{
		if (ec)
		{
			_read_buffer.consume(length);
			std::cerr << ec.message() << '\n';
		}
		else
		{
			const std::string archiveData(buffers_begin(_read_buffer.data()),
			                              buffers_begin(_read_buffer.data()) + length);
			std::istringstream archiveStream(archiveData);
			boost::archive::text_iarchive ia(archiveStream);
			ClientData data;
			ia >> data;

			_read_buffer.consume(length);

			// TODO: fix starting host on pause, connect and start client, release pause to sync starting game, need to sync game on client start, mean connect into continuous game
			if (data.eventName == "Pos")
			{
				events->EmitEvent<const FPoint, const Direction>(
						"ClientReceived_" + data.who + data.eventName, data.pos, data.dir);
			}
			else if (data.eventName == "Shot")
			{
				events->EmitEvent<const Direction>("ClientReceived_" + data.who + data.eventName, data.dir);
			}
			else if (data.eventName == "Health")
			{
				events->EmitEvent<const int>("ClientReceived_" + data.who + data.eventName, data.health);
			}
			else if (data.eventName == "Dispose")
			{
				events->EmitEvent("ClientReceived_" + data.who + data.eventName);
			}
			else if (data.eventName == "BonusSpawn")
			{
				events->EmitEvent<const FPoint, const BonusType, const int>(
						"ClientReceived_BonusSpawn", data.pos, data.type, data.id);
			}
			else if (data.eventName == "BonusDeSpawn")
			{
				events->EmitEvent<const int>("ClientReceived_" + data.eventName, data.id);
			}
			else if (data.eventName == "FortressDied")
			{
				events->EmitEvent<const int>("ClientReceived_" + data.eventName, data.id);
			}
			else if (data.eventName == "FortressToSteel")
			{
				events->EmitEvent<const int>("ClientReceived_FortressToSteel", data.id);
			}
			else if (data.eventName == "FortressToBrick")
			{
				events->EmitEvent<const int>("ClientReceived_FortressToBrick", data.id);
			}
			else if (data.eventName == "OnHelmetActivate")
			{
				events->EmitEvent("ClientReceived_" + data.who + data.eventName);
			}
			else if (data.eventName == "OnHelmetDeactivate")
			{
				events->EmitEvent("ClientReceived_" + data.who + data.eventName);
			}
			else if (data.eventName == "OnStar")
			{
				events->EmitEvent("ClientReceived_" + data.who + data.eventName);
			}
			else if (data.eventName == "OnTank")
			{
				events->EmitEvent<const std::string&, const std::string&>(
						"ClientReceived_" + data.eventName, data.who, data.fraction);
			}
			else if (data.eventType == "Statistics")
			{
				events->EmitEvent<const std::string&, const std::string&, const std::string&>(
						"ClientReceived_" + data.eventType, data.eventName, data.who, data.fraction);
			}
			else if (data.eventName == "OnGrenade")
			{
				events->EmitEvent<const std::string&, const std::string&>(
						"ClientReceived_" + data.eventName, data.who, data.fraction);
			}
			else if (data.eventName == "KeyState")//key input
			{
				events->EmitEvent(data.who);
			}

			// Since we want to keep listening, initiate reading again
			this->ReadResponse();
		}
	};

	boost::asio::async_read_until(_socket, _read_buffer, "\n\n", std::move(lambda));
}

void Client::SendKeyState(const std::string& state)
{
	// auto self(shared_from_this());
	ClientData data;
	data.health = 1;
	data.eventName = state;
	data.names = {"Name1", "Name2"};

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	// Безопасно добавляем сообщение в буфер для записи.
	{
		std::ostream os(&_write_buffer);
		os << archiveStream.str() + "\n\n";
	}

	boost::asio::async_write(
			_socket,
			_write_buffer,
			[this](const boost::system::error_code& ec, const std::size_t length)
			{
				_write_buffer.consume(length);// Now we can consume the written data
				if (!ec)
				{
					// Response send success!
					// this->ReadResponse();
				}
			});
}
