#include "../../headers/network/Server.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/TankType.h"
#include "../../headers/network/commands/BonusDeSpawn.h"
#include "../../headers/network/commands/BonusSpawn.h"
#include "../../headers/network/commands/Dispose.h"
#include "../../headers/network/commands/FortressChange.h"
#include "../../headers/network/commands/HealthChange.h"
#include "../../headers/network/commands/KeyStateChange.h"
#include "../../headers/network/commands/PositionChange.h"
#include "../../headers/network/commands/RespawnTank.h"
#include "../../headers/network/commands/StatisticsChange.h"
#include "../../headers/network/commands/TankShot.h"

#include <fstream>
#include <iostream>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/uuid/uuid.hpp>

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
	_events->AddListener("Pause_Pressed", _name,
	                     [this]() { SendCommand(std::make_shared<KeyStateChange>("Pause_Pressed")); });
	_events->AddListener("Pause_Released", _name,
	                     [this]() { SendCommand(std::make_shared<KeyStateChange>("Pause_Released")); });

	_events->AddListener<const std::string&, const boost::uuids::uuid>(
			"ServerSend_FortressChange", _name,
			[this](const std::string& state, const boost::uuids::uuid uuid)
			{
				SendCommand(std::make_shared<FortressChange>(state, uuid));
			});

	_events->AddListener<const std::string&, const FPoint, const Direction, const boost::uuids::uuid>(
			"ServerSend_Pos", _name,
			[this](const std::string& who, const FPoint pos, const Direction dir, const boost::uuids::uuid uuid)
			{
				SendCommand(std::make_shared<PositionChange>(who, pos, dir, uuid));
			});

	_events->AddListener<const std::string&, const Direction, const boost::uuids::uuid>(
			"ServerSend_Shot"/*TODO: rename_Shot bulletSpawn*/, _name,
			[this](const std::string& who, const Direction dir, const boost::uuids::uuid uuid)
			{
				SendCommand(std::make_shared<TankShot>(who, dir, uuid));
			});

	_events->AddListener<const std::string&, const int, const boost::uuids::uuid>(
			"ServerSend_Health", _name,
			[this](const std::string& who, const int health, const boost::uuids::uuid uuid)
			{
				SendCommand(std::make_shared<HealthChange>(who, health, uuid));
			});

	_events->AddListener<const boost::uuids::uuid>(
			"ServerSend_Dispose", _name,
			[this](/*TODO: add who,*/const boost::uuids::uuid uuid)
			{
				SendCommand(std::make_shared<Dispose>("Bullet", uuid));
			});

	_events->AddListener<const std::string&, const std::string&, const std::string&>(
			"ServerSend_Statistics", _name,//TODO: refactor statistics to send actual value not increment
			[this](const std::string& eventName, const std::string& author, const std::string& fraction)
			{
				SendCommand(std::make_shared<StatisticsChange>(eventName, author, fraction));
			});

	_events->AddListener<const TankType, const boost::uuids::uuid>(
			"ServerSend_RespawnTank", _name,
			[this](const TankType type, const boost::uuids::uuid uuid)
			{
				SendCommand(std::make_shared<RespawnTank>(type, uuid));
			});

	SubscribeBonus();
}

void Server::SubscribeBonus() const
{
	_events->AddListener<const FPoint, const BonusType, const boost::uuids::uuid>(
			"ServerSend_BonusSpawn", _name,
			[this](const FPoint pos, const BonusType type, const boost::uuids::uuid uuid)
			{
				SendCommand(std::make_shared<BonusSpawn>(pos, type, uuid));
			});
	_events->AddListener<const boost::uuids::uuid>(
			"ServerSend_BonusDeSpawn", _name,
			[this](const boost::uuids::uuid uuid)
			{
				SendCommand(std::make_shared<BonusDeSpawn>(uuid));
			});
	//TODO: clien obstacle spawn with uuid
	//TODO: clien bonus spawn with uuid


	// _events->AddListener<const std::string&>("ServerSend_OnHelmetActivate", _name, [this](const std::string& who)
	// {
	// 	this->OnHelmetActivate(who);//TODO: refactor to SendCommand(std::make_shared<
	// });
	// _events->AddListener<const std::string&>("ServerSend_OnHelmetDeactivate", _name, [this](const std::string& who)
	// {
	// 	this->OnHelmetDeactivate(who);//TODO: refactor to SendCommand(std::make_shared<
	// });
	// _events->AddListener<const std::string&>("ServerSend_OnStar", _name, [this](const std::string& who)
	// {
	// 	this->OnStar(who);//TODO: refactor to SendCommand(std::make_shared<
	// });
	// _events->AddListener<const std::string&, const std::string&>(
	// 		"ServerSend_OnTank", _name, [this](const std::string& author, const std::string& fraction)
	// 		{
	// 			this->OnTank(author, fraction);//TODO: refactor to SendCommand(std::make_shared<
	// 		});
	//
	// _events->AddListener<const std::string&, const std::string&>(
	// 		"ServerSend_OnGrenade", _name, [this](const std::string& author, const std::string& fraction)
	// 		{
	// 			this->OnGrenade(author, fraction);//TODO: refactor to SendCommand(std::make_shared<
	// 		});
}

void Server::Unsubscribe() const
{
	_events->RemoveListener("Pause_Pressed", _name);
	_events->RemoveListener("Pause_Released", _name);

	_events->RemoveListener<const std::string&, const FPoint, const Direction, const boost::uuids::uuid>(
			"ServerSend_Pos", _name);
	_events->RemoveListener<const std::string&, const int, const boost::uuids::uuid>("ServerSend_Health", _name);
	_events->RemoveListener<const boost::uuids::uuid>("ServerSend_Dispose", _name);
	_events->RemoveListener<const std::string&, const Direction, const boost::uuids::uuid>("ServerSend_Shot", _name);
	_events->RemoveListener<const std::string&, const std::string&, const std::string&>("ServerSend_Statistics", _name);

	UnsubscribeBonus();
}

void Server::UnsubscribeBonus() const
{
	_events->RemoveListener<const FPoint, const BonusType, const boost::uuids::uuid>("ServerSend_BonusSpawn", _name);
	_events->RemoveListener<const boost::uuids::uuid>("ServerSend_BonusDeSpawn", _name);

	_events->RemoveListener<const std::string&, const boost::uuids::uuid>("ServerSend_FortressChange", _name);

	// _events->RemoveListener<const std::string&>("ServerSend_OnHelmetActivate", _name);
	// _events->RemoveListener<const std::string&>("ServerSend_OnHelmetDeactivate", _name);
	// _events->RemoveListener<const std::string&>("ServerSend_OnStar", _name);
	// _events->RemoveListener<const std::string&, const std::string&>("ServerSend_OnTank", _name);
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

void Server::SendCommand(const std::shared_ptr<Command>& command) const
{
	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << command;

	this->SendToAll(archiveStream.str() + "\n\n");
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
