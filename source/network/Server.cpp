#include "../../headers/network/Server.h"
#include "../../headers/components/EventSystem.h"

#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>

// std::ofstream error_log("error_log.txt");

Session::Session(tcp::socket sock, std::shared_ptr<EventSystem> events)
	: _socket(std::move(sock)), _events(std::move(events)) {}

Session::~Session()
{
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
				std::cerr << "Error closing socket socket: " << ec.message() << std::endl;
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception in ~Session: " << e.what() << '\n';
	}
	catch (...)
	{
		std::cerr << "Unknown error in ~Session" << '\n';
	}
}

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
				_readBuffer.consume(length);
				std::cerr << "DoRead error ..." << '\n';
			}
			else
			{
				const std::string archiveData(buffers_begin(_readBuffer.data()),
				                              buffers_begin(_readBuffer.data()) + length);
				std::istringstream archiveStream(archiveData);
				boost::archive::text_iarchive ia(archiveStream);

				ServerData data;
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

				_readBuffer.consume(length);
				DoRead();
			}
		};

		boost::asio::async_read_until(_socket, _readBuffer, "\n\n", lambda);
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
			std::ostream os(&_writeBuffer);
			os << message;
		}

		auto self(shared_from_this());
		auto lambda = [self](const boost::system::error_code& ec, const std::size_t length)
		{
			self->_writeBuffer.consume(length);// Now we can consume the written data

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
		boost::asio::async_write(_socket, _writeBuffer.data(), std::move(lambda));
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

	Subscribe();
}

Server::~Server()
{
	// error_log.close();

	if (_acceptor.is_open())
	{
		_acceptor.close();
	}

	Unsubscribe();
}

void Server::Subscribe() const
{
	_events->AddListener("Pause_Pressed", _name, [this]() { this->SendKeyState("Pause_Pressed"); });
	_events->AddListener("Pause_Released", _name, [this]() { this->SendKeyState("Pause_Released"); });

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
			"ServerSend_Pos", _name, [this](const std::string& who, const FPoint pos, const Direction dir)
			{
				this->SendPos(who, pos, dir);
			});

	_events->AddListener<const std::string&, const int>(
			"ServerSend_Health", _name, [this](const std::string& who, const int health)
			{
				this->SendHealth(who, health);
			});

	_events->AddListener<const int>("ServerSend_Dispose", _name, [this](const int bulletId)
	{
		this->SendDispose("Bullet" + std::to_string(bulletId));
	});

	//TODO: rename_Shot bulletSpawn
	_events->AddListener<const std::string&, const Direction>(
			"ServerSend_Shot", _name, [this](const std::string& who, const Direction dir)
			{
				this->SendShot(who, dir);
			});

	_events->AddListener<const std::string&, const std::string&, const std::string&>(
			"ServerSend_Statistics", _name,
			[this](const std::string& eventName, const std::string& author, const std::string& fraction)
			{
				this->OnStatisticsChange(eventName, author, fraction);
			});

	SubscribeBonus();
}

void Server::SubscribeBonus() const
{
	_events->AddListener<const std::string&, const FPoint, const BonusType, const int>(
			"ServerSend_BonusSpawn", _name,
			[this](const std::string& who, const FPoint pos, const BonusType type, const int id)
			{
				this->SendBonusSpawn(who, pos, type, id);
			});
	_events->AddListener<const int>("ServerSend_BonusDeSpawn", _name, [this](const int id)
	{
		this->SendBonusDeSpawn(id);
	});

	_events->AddListener<const std::string&>("ServerSend_OnHelmetActivate", _name, [this](const std::string& who)
	{
		this->OnHelmetActivate(who);
	});
	_events->AddListener<const std::string&>("ServerSend_OnHelmetDeactivate", _name, [this](const std::string& who)
	{
		this->OnHelmetDeactivate(who);
	});
	_events->AddListener<const std::string&>("ServerSend_OnStar", _name, [this](const std::string& who)
	{
		this->OnStar(who);
	});
	_events->AddListener<const std::string&, const std::string&>(
			"ServerSend_OnTank", _name, [this](const std::string& author, const std::string& fraction)
			{
				this->OnTank(author, fraction);
			});

	_events->AddListener<const std::string&, const std::string&>(
			"ServerSend_OnGrenade", _name, [this](const std::string& author, const std::string& fraction)
			{
				this->OnGrenade(author, fraction);
			});
}

void Server::Unsubscribe() const
{
	_events->RemoveListener("Pause_Pressed", _name);
	_events->RemoveListener("Pause_Released", _name);

	_events->RemoveListener<const std::string&, const FPoint, const Direction>("ServerSend_Pos", _name);
	_events->RemoveListener<const std::string&, const int>("ServerSend_Health", _name);
	_events->RemoveListener<const int>("ServerSend_Dispose", _name);
	_events->RemoveListener<const std::string&, const Direction>("ServerSend_Shot", _name);
	_events->RemoveListener<const std::string&, const std::string&, const std::string&>("ServerSend_Statistics", _name);

	UnsubscribeBonus();
}

void Server::UnsubscribeBonus() const
{
	_events->RemoveListener<const std::string&, const FPoint, const BonusType, const int>(
 			"ServerSend_BonusSpawn", _name);
 	_events->RemoveListener<const int>("ServerSend_BonusDeSpawn", _name);

	_events->RemoveListener<const std::string&>("ServerSend_OnHelmetActivate", _name);
	_events->RemoveListener<const std::string&>("ServerSend_OnHelmetDeactivate", _name);
	_events->RemoveListener<const std::string&>("ServerSend_OnStar", _name);
	_events->RemoveListener<const std::string&, const std::string&>("ServerSend_OnTank", _name);
	_events->RemoveListener<const std::string&>("ServerSend_OnGrenade", _name);
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
				_sessions.emplace_back(std::make_shared<Session>(std::move(socket), _events));
				if (const auto& lastSession = _sessions.back(); lastSession)
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
	for (const auto& session: _sessions)
		session->DoWrite(message);
}

void Server::SendDispose(const std::string& bulletName) const
{
	ServerData data;
	data.eventName = "Dispose";
	data.who = bulletName;

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendKeyState(const std::string& who) const
{
	ServerData data;
	data.eventName = "KeyState";
	data.who = who;

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendShot(const std::string& who, const Direction dir) const
{
	ServerData data;
	data.eventName = "Shot";//TODO: refactor bullet pool event to avoid un sync situation
	data.who = who;
	data.dir = dir;

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendKeyState(const std::string& state, const FPoint newPos, const Direction dir) const
{
	ServerData data;
	data.eventName = state;
	data.pos = newPos;
	data.dir = dir;

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendPos(const std::string& who, const FPoint pos, const Direction dir) const
{
	ServerData data;
	data.who = who;
	data.eventName = "Pos";
	data.pos = pos;
	data.dir = dir;

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendBonusSpawn(const std::string& who, const FPoint pos, const BonusType type, const int id) const
{
	ServerData data;

	data.who = who;//TODO: remove unused param
	data.eventName = "BonusSpawn";
	data.pos = pos;
	data.id = id;
	data.type = type;

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendBonusDeSpawn(const int id) const
{
	ServerData data;

	data.eventName = "BonusDeSpawn";
	data.id = id;

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendHealth(const std::string& who, const int health) const
{
	ServerData data;
	data.health = health;
	data.who = who;
	data.eventName = "Health";

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::OnHelmetActivate(const std::string& who) const
{
	ServerData data;
	data.who = who;
	data.eventName = "OnHelmetActivate";

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::OnHelmetDeactivate(const std::string& who) const
{
	ServerData data;
	data.who = who;
	data.eventName = "OnHelmetDeactivate";

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::OnStar(const std::string& who) const
{
	ServerData data;
	data.who = who;
	data.eventName = "OnStar";

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::OnTank(const std::string& who, const std::string& fraction) const
{
	ServerData data;
	data.who = who;
	data.eventName = "OnTank";
	data.fraction = fraction;

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::OnGrenade(const std::string& who, const std::string& fraction) const
{
	ServerData data;
	data.who = who;
	data.eventName = "OnGrenade";
	data.fraction = fraction;

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::OnStatisticsChange(const std::string& eventName, const std::string& author,
                                const std::string& fraction) const
{
	ServerData data;
	data.eventType = "Statistics";//TODO: refactor statistics to send actual value not increment
	data.eventName = eventName;
	data.who = author;
	data.fraction = fraction;

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendFortressDied(const int id) const
{
	ServerData data;
	data.id = id;
	data.eventName = "FortressDied";

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendFortressToBrick(const int id) const
{
	ServerData data;
	data.id = id;
	data.eventName = "FortressToBrick";

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}

void Server::SendFortressToSteel(const int id) const
{
	ServerData data;
	data.id = id;
	data.eventName = "FortressToSteel";

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	SendToAll(archiveStream.str() + "\n\n");
}
