#include "Client.h"

#include <fstream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

std::ofstream error_log("error_log_client.txt");

Client::Client(boost::asio::io_service& ioService, const std::string& host, const std::string& port,
               std::shared_ptr<EventSystem> events)
	: _socket(ioService),
	  _events{std::move(events)},
	  _name{"Client"}
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

	tcp::resolver resolver(ioService);
	const auto endpointIterator = resolver.resolve({host, port});
	boost::asio::async_connect(
			_socket,
			endpointIterator,
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
			_read_buffer.consume(length);// Now we can consume the written data
			// Обработка ошибки
			error_log << ec.message() << '\n';
		}
		else
		{
			const std::string archiveData(boost::asio::buffer_cast<const char*>(this->_read_buffer.data()),
			                              length);
			std::istringstream archiveStream(archiveData);
			boost::archive::text_iarchive ia(archiveStream);
			Data data;
			ia >> data;

			_read_buffer.consume(length);// Now we can consume the written data

			//TODO: replicate tank died and spawn, now only next move update them or new health update health
			// TODO: fix starting host on pause, connect and start client, release pause to sync starting game, need to sync game on client start, mean connect into continuous game 
			if (data.eventName == "_NewHealth")
			{
				events->EmitEvent<const int>("Net_" + data.objectName + data.eventName, data.health);
			}
			else if (data.eventName.ends_with("_Shot"))
			{
				events->EmitEvent<const Direction>("Net_" + data.eventName, data.direction);
			}
			else if (data.eventName.starts_with("_Dispose"))
			{
				events->EmitEvent<const FPoint, const Direction>("Net_" + data.eventName, data.newPos, data.direction);
			}
			else if (data.eventName == "_NewPos")
			{
				events->EmitEvent<const FPoint, const Direction>("Net_" + data.objectName + data.eventName, data.newPos, data.direction);
			}
			else
			{
				events->EmitEvent("Net_" + data.eventName);
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
	Data data;
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
