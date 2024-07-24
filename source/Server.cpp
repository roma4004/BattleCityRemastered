#include "../headers/Server.h"

#include <fstream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

std::ofstream error_log("error_log.txt");

Session::Session(tcp::socket sock, std::shared_ptr<EventSystem> events)
	: _socket(std::move(sock)), _events(std::move(events)) {}

void Session::Start()
{
	try
	{
		DoRead();
	}
	catch (const std::exception& e)
	{
		error_log << e.what() << '\n';
	}
	catch (...)
	{
		error_log << "error ..." << '\n';
	}
}

void Session::DoRead()
{
	try
	{
		auto self(shared_from_this());
		auto lambda = [this, events = _events](const boost::system::error_code& ec, const std::size_t length)
		{
			if (ec)
			{
				_read_buffer.consume(length);
				error_log << "DoRead error ..." << '\n';
			}
			else
			{
				const std::string archiveData(boost::asio::buffer_cast<const char*>(_read_buffer.data()), length);
				std::istringstream archiveStream(archiveData);
				boost::archive::text_iarchive ia(archiveStream);

				Data data;
				ia >> data;

				// std::cout << "Received data:\n";
				// std::cout << "Id: " << data.id << "\n";
				// std::cout << "Name: " << data.name << "\n";

				//TODO: check if key allowed to receive from client
				events->EmitEvent("Net_" + data.eventName);

				// std::cout << "Names: ";
				// for (auto& name: data.names)
				// 	std::cout << name << " ";
				// std::cout << "\n";

				// // Respond back to client
				// self->DoWrite({123, "Test", {"Name1", "Name2"}});

				_read_buffer.consume(length);
				DoRead();
			}
		};

		boost::asio::async_read_until(_socket, _read_buffer, "\n\n", lambda);
	}
	catch (const std::exception& e)
	{
		error_log << "Exception in DoWrite: " << e.what() << '\n';
	}
	catch (...)
	{
		error_log << "error ..." << '\n';
	}
}

void Session::DoWrite(const std::string& message)
{
	try
	{
		if (!_socket.is_open())
		{
			error_log << "Socket is not open. Cannot write.";
			return;
		}

		// Безопасно добавляем сообщение в буфер для записи.
		{
			std::ostream os(&_write_buffer);
			os << message;
		}

		auto self(shared_from_this());
		auto lambda = [self](const boost::system::error_code& ec, const std::size_t length)
		{
			self->_write_buffer.consume(length);// Now we can consume the written data

			if (ec)
			{
				error_log << "Write error: " << ec.message() << '\n';
				self->_socket.close();
				//TODO: need handle close connection and delete session
			}
			else
			{
				// You can handle custom success write case here
				// If you want to keep the session alive add your process here. Like DoRead again.
				// self->DoRead();
			}
		};

		// Start async write operation
		boost::asio::async_write(_socket, _write_buffer.data(), std::move(lambda));
	}
	catch (const std::exception& e)
	{
		error_log << "Exception in DoWrite: " << e.what() << '\n';
	}
	catch (...)
	{
		error_log << "error ..." << '\n';
	}
}

Server::Server(boost::asio::io_service& ioService, const std::string& host, const std::string& port,
               std::shared_ptr<EventSystem> events)
	: _acceptor(ioService, tcp::endpoint(boost::asio::ip::address::from_string(host).to_v4(),
	                                     static_cast<unsigned short>(std::stoul(port)))),
	  _events{std::move(events)},
	  _name{"Server"}
{
	DoAccept();

	_events->AddListener("Pause_Pressed", _name, [this]() { this->SendKeyState("Pause_Pressed"); });
	_events->AddListener("Pause_Released", _name, [this]() { this->SendKeyState("Pause_Released"); });

	_events->AddListener<const std::string, const std::string, const FPoint, const Direction>(
			"_NewPos",
			_name,
			[this](const std::string objectName, const std::string eventName, const FPoint newPos,
			       const Direction direction)
			{
				this->SendNewPos(objectName, eventName, newPos, direction);
			});

	_events->AddListener<const std::string, const std::string, const int>(
			"SendHealth",
			_name,
			[this](const std::string objectName, const std::string eventName, const int health)
			{
				this->SendHealth(objectName, eventName, health);
			});

	_events->AddListener<const int>(
			"Bullet_Dispose",
			_name,
			[this](const int bulletId)
			{
				this->SendKeyState("Bullet" + std::to_string(bulletId) + "_Dispose");
			});

	//TODO: rename_Shot bulletSpawn
	_events->AddListener<const Direction>("Enemy1_Shot", _name, [this](const Direction direction)
	{
		this->SendShot("Enemy1_Shot", direction);
	});
	_events->AddListener<const Direction>("Enemy2_Shot", _name, [this](const Direction direction)
	{
		this->SendShot("Enemy2_Shot", direction);
	});
	_events->AddListener<const Direction>("Enemy3_Shot", _name, [this](const Direction direction)
	{
		this->SendShot("Enemy3_Shot", direction);
	});
	_events->AddListener<const Direction>("Enemy4_Shot", _name, [this](const Direction direction)
	{
		this->SendShot("Enemy4_Shot", direction);
	});

	_events->AddListener<const Direction>("Player1_Shot", _name, [this](const Direction direction)
	{
		this->SendShot("Player1_Shot", direction);
	});
	_events->AddListener<const Direction>("Player2_Shot", _name, [this](const Direction direction)
	{
		this->SendShot("Player2_Shot", direction);
	});
}

Server::~Server()
{
	error_log.close();

	_events->RemoveListener("Pause_Pressed", _name);
	_events->RemoveListener("Pause_Released", _name);

	_events->RemoveListener<const std::string, const std::string, const FPoint, const Direction>("_NewPos", _name);
	_events->RemoveListener<const std::string, const int>("SendHealth", _name);
	_events->RemoveListener<const int>("Bullet_Dispose", _name);

	_events->RemoveListener<const Direction>("Enemy1_Shot", _name);
	_events->RemoveListener<const Direction>("Enemy2_Shot", _name);
	_events->RemoveListener<const Direction>("Enemy3_Shot", _name);
	_events->RemoveListener<const Direction>("Enemy4_Shot", _name);

	_events->RemoveListener<const Direction>("Player1_Shot", _name);
	_events->RemoveListener<const Direction>("Player2_Shot", _name);
}

void Server::DoAccept()
{
	_acceptor.async_accept([this](const boost::system::error_code& ec, tcp::socket socket)
	{
		if (ec)
		{
			error_log << "Accept error: " << ec.message() << '\n';
		}
		else
		{
			try
			{
				sessions.emplace_back(std::make_shared<Session>(std::move(socket), _events));
				if (const auto& lastSession = sessions.back(); lastSession)
				{
					lastSession->Start();
				}
			}
			catch (const std::exception& e)
			{
				error_log << "Exception on new session start: " << e.what() << '\n';
			}
			catch (...)
			{
				error_log << "error ..." << '\n';
			}
			DoAccept();
		}
	});
}

void Server::SendToAll(const std::string& message) const
{
	for (const auto& session: sessions)
		session->DoWrite(message);
}

void Server::SendKeyState(const std::string& state) const
{
	Data data;
	data.eventName = state;

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendKeyState(const std::string& state, FPoint newPos) const
{
	Data data;
	data.eventName = state;
	data.newPos = newPos;

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendShot(const std::string& state, Direction direction) const
{
	Data data;
	data.eventName = state;
	data.direction = direction;

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendKeyState(const std::string& state, const FPoint newPos, const Direction direction) const
{
	Data data;
	data.eventName = state;
	data.newPos = newPos;
	data.direction = direction;

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendNewPos(const std::string& objectName, const std::string& eventName, const FPoint newPos,
                        const Direction direction) const
{
	Data data;
	data.objectName = objectName;
	data.eventName = eventName;
	data.newPos = newPos;
	data.direction = direction;

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendHealth(const std::string& objectName, const std::string& eventName, const int health) const
{
	Data data;
	data.health = health;
	data.objectName = objectName;
	data.eventName = eventName;

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}
