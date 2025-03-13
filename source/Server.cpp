#include "../headers/Server.h"
#include "../headers/EventSystem.h"

#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

// std::ofstream error_log("error_log.txt");

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
		std::cerr << e.what() << '\n';
	}
	catch (...)
	{
		std::cerr << "error ..." << '\n';
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
				std::cerr << "DoRead error ..." << '\n';
			}
			else
			{
				const std::string archiveData(buffers_begin(_read_buffer.data()),
				                              buffers_begin(_read_buffer.data()) + length);
				std::istringstream archiveStream(archiveData);
				boost::archive::text_iarchive ia(archiveStream);

				Data data;
				ia >> data;

				// std::cout << "Received data:\n";
				// std::cout << "Id: " << data.id << "\n";
				// std::cout << "Name: " << data.name << "\n";

				//TODO: check if key allowed to receive from client and strong validating net input
				events->EmitEvent("ServerReceive_" + data.eventName);

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
		std::cerr << "Exception in DoWrite: " << e.what() << '\n';
	}
	catch (...)
	{
		std::cerr << "error ..." << '\n';
	}
}

void Session::DoWrite(const std::string& message)
{
	try
	{
		if (!_socket.is_open())
		{
			std::cerr << "Socket is not open. Cannot write.";
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
				std::cerr << "Write error: " << ec.message() << '\n';
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
		std::cerr << "Exception in DoWrite: " << e.what() << '\n';
	}
	catch (...)
	{
		std::cerr << "error ..." << '\n';
	}
}

Server::Server(boost::asio::io_context& ioContext, const std::string& host, const std::string& port,
               std::shared_ptr<EventSystem> events)
	: _acceptor(ioContext, tcp::endpoint(boost::asio::ip::make_address(host).to_v4(),
	                                     static_cast<unsigned short>(std::stoul(port)))),
	  _events{std::move(events)},
	  _name{"Server"}
{
	DoAccept();

	_events->AddListener("Pause_Pressed", _name, [this]() { this->SendKeyState("Pause_Pressed"); });
	_events->AddListener("Pause_Released", _name, [this]() { this->SendKeyState("Pause_Released"); });

	_events->AddListener<const std::string>("ServerSend_TankDied", _name, [this](const std::string whoDied)
	{
		this->SendTankDied(whoDied);
	});

	_events->AddListener<const int>("ServerSend_FortressDied", _name, [this](const int id)
	{
		this->SendFortressDied(id);
	});

	_events->AddListener<const int>("ServerSend_FortressToBrick", _name, [this](const int id)
	{
		this->SendFortressToBrick(id);
	});

	_events->AddListener<const int>("ServerSend_FortressToSteel", _name, [this](const int id)
	{
		this->SendFortressToSteel(id);
	});

	_events->AddListener<const std::string&, const FPoint, const Direction>(
			"ServerSend_Pos", _name,
			[this](const std::string& objectName, const FPoint newPos, const Direction direction)
			{
				this->SendPos(objectName, newPos, direction);
			});

	_events->AddListener<const std::string&, const FPoint, const BonusTypeId, const int>(
			"ServerSend_BonusSpawn", _name,
			[this](const std::string& objectName, const FPoint spawnPos, const BonusTypeId typeId, const int id)
			{
				this->SendBonusSpawn(objectName, spawnPos, typeId, id);
			});

	_events->AddListener<const int>("ServerSend_BonusDeSpawn", _name, [this](const int id)
	{
		this->SendBonusDeSpawn(id);
	});

	_events->AddListener<const std::string, const int>(
			"ServerSend_Health", _name,
			[this](const std::string& objectName, const int health)
			{
				this->SendHealth(objectName, health);
			});

	_events->AddListener<const int>("ServerSend_Dispose", _name, [this](const int bulletId)
	{
		this->SendDispose("Bullet" + std::to_string(bulletId));
	});

	_events->AddListener("ServerSend_Enemy1_Died", _name, [this]() { this->SendTankDied("Enemy1"); });
	_events->AddListener("ServerSend_Enemy2_Died", _name, [this]() { this->SendTankDied("Enemy2"); });
	_events->AddListener("ServerSend_Enemy3_Died", _name, [this]() { this->SendTankDied("Enemy3"); });
	_events->AddListener("ServerSend_Enemy4_Died", _name, [this]() { this->SendTankDied("Enemy4"); });

	_events->AddListener("ServerSend_Player1_Died", _name, [this]() { this->SendTankDied("Player1"); });
	_events->AddListener("ServerSend_Player2_Died", _name, [this]() { this->SendTankDied("Player2"); });

	//TODO: rename_Shot bulletSpawn
	_events->AddListener<const Direction>("ServerSend_Enemy1Shot", _name, [this](const Direction direction)
	{
		this->SendShot("Enemy1", direction);
	});
	_events->AddListener<const Direction>("ServerSend_Enemy2Shot", _name, [this](const Direction direction)
	{
		this->SendShot("Enemy2", direction);
	});
	_events->AddListener<const Direction>("ServerSend_Enemy3Shot", _name, [this](const Direction direction)
	{
		this->SendShot("Enemy3", direction);
	});
	_events->AddListener<const Direction>("ServerSend_Enemy4Shot", _name, [this](const Direction direction)
	{
		this->SendShot("Enemy4", direction);
	});

	_events->AddListener<const Direction>("ServerSend_Player1Shot", _name, [this](const Direction direction)
	{
		this->SendShot("Player1", direction);
	});
	_events->AddListener<const Direction>("ServerSend_Player2Shot", _name, [this](const Direction direction)
	{
		this->SendShot("Player2", direction);
	});
}

Server::~Server()
{
	// error_log.close();

	_events->RemoveListener("Pause_Pressed", _name);
	_events->RemoveListener("Pause_Released", _name);

	_events->RemoveListener<const std::string&, const FPoint, const Direction>("ServerSend_Pos", _name);
	_events->RemoveListener<const std::string&, const FPoint, const BonusTypeId, const int>(
			"ServerSend_BonusSpawn", _name);
	_events->RemoveListener<const int>("ServerSend_BonusDeSpawn", _name);
	_events->RemoveListener<const std::string, const int>("ServerSend_Health", _name);
	_events->RemoveListener<const int>("ServerSend_Dispose", _name);

	_events->RemoveListener("Enemy1_Died", _name);
	_events->RemoveListener("Enemy2_Died", _name);
	_events->RemoveListener("Enemy3_Died", _name);
	_events->RemoveListener("Enemy4_Died", _name);

	_events->RemoveListener("Player1_Died", _name);
	_events->RemoveListener("Player2_Died", _name);

	_events->RemoveListener<const Direction>("Enemy1Shot", _name);
	_events->RemoveListener<const Direction>("Enemy2Shot", _name);
	_events->RemoveListener<const Direction>("Enemy3Shot", _name);
	_events->RemoveListener<const Direction>("Enemy4Shot", _name);

	_events->RemoveListener<const Direction>("Player1Shot", _name);
	_events->RemoveListener<const Direction>("Player2Shot", _name);

	_events->RemoveListener<const std::string>("ServerSend_TankDied", _name);
}

void Server::DoAccept()
{
	_acceptor.async_accept([this](const boost::system::error_code& ec, tcp::socket socket)
	{
		if (ec)
		{
			std::cerr << "Accept error: " << ec.message() << '\n';
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
				std::cerr << "Exception on new session start: " << e.what() << '\n';
			}
			catch (...)
			{
				std::cerr << "error ..." << '\n';
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

void Server::SendDispose(const std::string& bulletName) const
{
	Data data;
	data.eventName = "Dispose";
	data.objectName = bulletName;

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendKeyState(const std::string& state) const
{
	Data data;
	data.eventName = "KeyState";
	data.objectName = state;

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

//deprecated
void Server::SendKeyState(const std::string& state, const FPoint newPos) const
{
	Data data;
	data.eventName = state;
	data.newPos = newPos;

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendShot(const std::string& objectName, const Direction direction) const
{
	Data data;
	data.eventName = "Shot";
	data.objectName = objectName;
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

void Server::SendPos(const std::string& objectName, const FPoint newPos, const Direction direction) const
{
	Data data;
	data.objectName = objectName;
	data.eventName = "Pos";
	data.newPos = newPos;
	data.direction = direction;

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendBonusSpawn(const std::string& objectName, const FPoint spawnPos, const BonusTypeId typeId,
                            const int id) const
{
	Data data;

	data.objectName = objectName;
	data.eventName = "BonusSpawn";
	data.newPos = spawnPos;
	data.id = id;
	data.typeId = typeId;

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendBonusDeSpawn(const int id) const
{
	Data data;

	data.eventName = "BonusDeSpawn";
	data.id = id;

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendHealth(const std::string& objectName, const int health) const
{
	Data data;
	data.health = health;
	// data.id = objectId;
	data.objectName = objectName;
	data.eventName = "Health";

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendTankDied(const std::string& objectName) const
{
	Data data;
	data.objectName = objectName;
	data.eventName = "TankDied";

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendFortressDied(const int id) const
{
	Data data;
	data.id = id;
	data.eventName = "FortressDied";

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendFortressToBrick(const int id) const
{
	Data data;
	data.id = id;
	data.eventName = "FortressToBrick";

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendFortressToSteel(const int id) const
{
	Data data;
	data.id = id;
	data.eventName = "FortressToSteel";

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}
