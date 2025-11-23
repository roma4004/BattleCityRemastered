#include "network/Server.h"
#include "components/EventSystem.h"
#include "entities/ObjRectangle.h"
#include "enums/TankType.h"
#include "network/commands/AnimationCreate.h"
#include "network/commands/BonusDeSpawn.h"
#include "network/commands/BonusSpawn.h"
#include "network/commands/BonusStatus.h"
#include "network/commands/CommandBatch.h"
#include "network/commands/Dispose.h"
#include "network/commands/FortressChange.h"
#include "network/commands/HealthChange.h"
#include "network/commands/KeyStateChange.h"
#include "network/commands/ObstacleSpawn.h"
#include "network/commands/PositionChange.h"
#include "network/commands/RespawnTank.h"
#include "network/commands/StatisticsChange.h"
#include "network/commands/TankOnOff.h"
#include "network/commands/TankShot.h"
#include "utils/NetworkLogger.h"
#include <fstream>
#include <iostream>
#include <mutex>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/uuid/uuid.hpp>

// std::ofstream error_log("error_log.txt");

using buuid = boost::uuids::uuid;

Session::Session(tcp::socket sock, const std::shared_ptr<EventSystem>& events)
	: _socket(std::move(sock)), _events(events) {}

Session::~Session()
{
	try
	{
		if (_socket.is_open())
		{
			boost::system::error_code ec;

			boost::system::error_code shutdownResut = _socket.shutdown(tcp::socket::shutdown_both, ec);
			if (ec)
			{
				std::cerr << "Error during socket shutdown: " << ec.message() << '\n';
			}

			boost::system::error_code closeResut = _socket.close(ec);
			if (ec)
			{
				std::cerr << "Error closing socket socket: " << ec.message() << '\n';
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
				std::cerr << "DoRead error ..." << ec << '\n';
			}
			else
			{
				const std::string archiveData(buffers_begin(_readBuffer.data()),
				                              buffers_begin(_readBuffer.data()) + length);
				std::istringstream archiveStream(archiveData);
				boost::archive::text_iarchive ia(archiveStream);

				ServerData data;
				ia >> data;

				// NetworkLogger::LogServerReceive(data.eventName);

				// std::cout << "Received data:\n";
				// std::cout << "Id: " << data.id << "\n";
				// std::cout << "Name: " << data.name << "\n";

				if (data.eventName == "ClientReadyToPlay")
				{
					events->EmitEvent("ClientReadyToStartGame");
				}
				else
				{
					//TODO: check if key allowed to receive from client and strong validating net input
					events->EmitEvent("ServerReceive_" + data.eventName);
				}

				// std::cout << "Names: ";
				// for (auto& name: data.names)
				// 	std::cout << name << " ";
				// std::cout << "\n";

				// // Respond back to a client
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
				if (ec == boost::asio::error::eof || ec == boost::asio::error::operation_aborted)
				{
					std::cout << "Connection closed normally" << '\n';
				}
				else
				{
					std::cerr << "Write error: " << ec.message() << '\n';
					//TODO: need handle close connection and delete session
				}

				self->_socket.close();
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
               const std::shared_ptr<EventSystem>& events)
	: _acceptor(ioContext, tcp::endpoint(boost::asio::ip::make_address(host).to_v4(),
	                                     static_cast<unsigned short>(std::stoul(port)))),
	  _events{events},
	  _name{"Server"},
	  _isRunning{true}
{
	_batch = std::make_shared<CommandBatch>();
	DoAccept();
	StartSendThread();
	Subscribe();
}

void Server::StartSendThread()
{
	_sendThread = std::thread([this]()
	{
		while (_isRunning)
		{
			std::shared_ptr<CommandBatch> batch;

			{
				std::unique_lock<std::mutex> lock(_sendQueueMutex);
				_sendCondition.wait(lock, [this]
				{
					return !_sendQueue.empty() || !_isRunning;
				});

				if (!_isRunning)
					break;

				if (_sendQueue.empty())
					continue;

				batch = _sendQueue.front();
				_sendQueue.pop();
			}

			if (batch && !batch->IsEmpty())
			{
				try
				{
					SendCommand(batch);
				}
				catch (const std::exception& e)
				{
					std::cerr << "Exception in send thread: " << e.what() << '\n';

					// retry send
					std::scoped_lock lock(_sendQueueMutex);
					_sendQueue.push(batch);
				}
			}
		}
	});
}

void Server::StopSendThread()
{
	{
		std::scoped_lock lock(_sendQueueMutex);
		_isRunning = false;
	}

	_sendCondition.notify_one();

	if (_sendThread.joinable())
	{
		_sendThread.join();
	}
}

Server::~Server()
{
	// error_log.close();
	StopSendThread();

	if (_acceptor.is_open())
	{
		_acceptor.close();
	}

	Unsubscribe();
}

void Server::Subscribe()
{
	// _events->AddListener("Server_StartFrame", _name, [this](){});

	_events->AddListener("Server_EndFrame", _name, [this]()
	{
		std::scoped_lock lock(_batchWriteMutex, _sendQueueMutex);

		// NetworkLogger::WriteLog("===Server_EndFrame");
		// std::shared_ptr<CommandBatch> toSend{nullptr};
		// toSend = _batch;
		// _batch = std::make_shared<CommandBatch>();
		// SendCommand(toSend);

		//TODO: queue sending work fine but need recheck before release 
		_sendQueue.emplace(_batch);
		_sendCondition.notify_one();
		_batch = std::make_shared<CommandBatch>();

		// int i = 0;
		// for (auto& commands = toSend->GetCommands();
		// 	auto command: commands)
		// {
		// 	auto classNameW = std::string(command->GetClassNameW());
		// 	// NetworkLogger::WriteLog("Server_bach_command i=" + std::to_string(i++) + " " + classNameW);
		// 	SendCommand(command);
		// }
		// NetworkLogger::WriteLog("Server_EndFrame===");

		// if (toSend && toSend->GetCommands().size() > 0)
		// {
		// 	{
		// 		std::scoped_lock lock(_sendQueueMutex);
		// 		_sendQueue.push(toSend);
		// 	}
		// 	_sendCondition.notify_one();// Повідомляємо потік відправки
		// }

		//Mark that one batch need to be sent (or send immediately)
		// std::scoped_lock lock(_batchWriteMutex);
		// if (_batch.get() != nullptr && _batch->GetCommands().size() > 0)
		// {
		// 	SendCommand(_batch);
		// }
	});

	_events->AddListener("Pause_Pressed", _name, [this]()
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<KeyStateChange>("Pause_Pressed"));
	});

	_events->AddListener("Pause_Released", _name, [this]()
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<KeyStateChange>("Pause_Released"));
	});

	_events->AddListener("ServerSend_FortressChange", _name, [this](const std::string& state, const buuid& uuid)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<FortressChange>(state, uuid));
	});

	_events->AddListener(
			"ServerSend_Pos", _name,
			[this](const std::string& who, const FPoint pos, const Direction dir, const buuid& uuid)
			{
				// NetworkLogger::WriteLog("Server_positionChange add before:" + std::to_string(_batch->GetSize()));
				// std::scoped_lock lock(_batchWriteMutex);
				const auto positionChange = std::make_shared<PositionChange>(who, pos, dir, uuid);
				// auto classNameW = std::string(positionChange->GetClassNameW());
				// NetworkLogger::WriteLog("Server_positionChange send:"+classNameW);
				_batch->AddCommand(positionChange);
				// NetworkLogger::WriteLog("Server_positionChange add after:" + std::to_string(_batch->GetSize()));
			});

	_events->AddListener(
			"ServerSend_Shot", _name,
			[this](const std::string& who, const Direction dir, const buuid& uuid)
			{
				std::scoped_lock lock(_batchWriteMutex);
				_batch->AddCommand(std::make_shared<TankShot>(who, dir, uuid));
			});

	_events->AddListener("ServerSend_Health", _name, [this](const std::string& who, const int health, const buuid& uuid)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<HealthChange>(who, health, uuid));
	});

	_events->AddListener("ServerSend_Dispose", _name, [this](/*TODO: add who,*/const buuid& uuid)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<Dispose>("Bullet", uuid));
	});

	_events->AddListener(
			"ServerSend_Statistics", _name,//TODO: refactor statistics to send actual value not increment
			[this](const std::string& eventName, const std::string& author, const std::string& fraction)
			{
				std::scoped_lock lock(_batchWriteMutex);
				_batch->AddCommand(std::make_shared<StatisticsChange>(eventName, author, fraction));
			});

	_events->AddListener("ServerSend_RespawnTank", _name, [this](const TankType type, const buuid& uuid)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<RespawnTank>(type, uuid));
	});

	_events->AddListener(
			"ServerSend_ObstacleSpawn", _name,
			[this](const ObjRectangle rect, const ObstacleType type, const buuid& uuid)
			{
				std::scoped_lock lock(_batchWriteMutex);
				_batch->AddCommand(std::make_shared<ObstacleSpawn>(rect, type, uuid));
			});
	//TODO: write obstacle dispose

	_events->AddListener(
			"ServerSend_AnimationCreate", _name,
			[this](const AnimationType type, const ObjRectangle rect, const std::string& name, const int color)
			{
				//TODO: fix multiple spawn bullet explosion animation
				std::scoped_lock lock(_batchWriteMutex);
				_batch->AddCommand(std::make_shared<AnimationCreate>(type, rect, name, color));
			});

	_events->AddListener(
			"ServerSend_OnTankOnOff", _name,
			[this](const buuid& uuid, const bool isEnabled, std::string name)
			{
				std::scoped_lock lock(_batchWriteMutex);
				_batch->AddCommand(std::make_shared<TankOnOff>(uuid, isEnabled, std::move(name)));
			});

	SubscribeBonus();
}

void Server::SubscribeBonus()
{
	_events->AddListener(
			"ServerSend_BonusSpawn", _name,
			[this](const FPoint pos, const BonusType type, const buuid& uuid)
			{
				std::scoped_lock lock(_batchWriteMutex);
				_batch->AddCommand(std::make_shared<BonusSpawn>(pos, type, uuid));
			});

	_events->AddListener("ServerSend_BonusDeSpawn", _name, [this](const buuid& uuid)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<BonusDeSpawn>(uuid));
	});

	_events->AddListener("ServerSend_OnBonusHelmet", _name, [this](const std::string& name, const bool isActive)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<BonusStatus>(name, BonusType::Helmet, isActive));
		//TODO: rewrite other bonus status effect changes just like this OnBonusHelmet
	});

	//TODO: client obstacle spawn with uuid
	//TODO: client bonus spawn with uuid

	// _events->AddListener("ServerSend_OnStar", _name, [this](const std::string& who)
	// {
	// 	this->OnStar(who);//TODO: refactor to SendCommand(std::make_shared<
	// });
	// _events->AddListener("ServerSend_OnCaliber", _name, [this](const std::string& who)
	// {
	// 	this->OnCaliber(who);//TODO: refactor to SendCommand(std::make_shared<
	// });
	// _events->AddListener("ServerSend_OnTank", _name, [this](const std::string& author, const std::string& fraction)
	// {
	// 	this->OnTank(author, fraction);//TODO: refactor to SendCommand(std::make_shared<
	// });
	//
	// _events->AddListener(
	// 		"ServerSend_OnGrenade", _name, [this](const std::string& author, const std::string& fraction)
	// 		{
	// 			this->OnGrenade(author, fraction);//TODO: refactor to SendCommand(std::make_shared<
	// 		});
}

void Server::Unsubscribe() const
{
	_events->RemoveListener("Pause_Pressed", _name);
	_events->RemoveListener("Pause_Released", _name);
	// _events->RemoveListener("Server_StartFrame", _name);
	_events->RemoveListener("Server_EndFrame", _name);

	_events->RemoveListener("ServerSend_Pos", _name);
	_events->RemoveListener("ServerSend_Health", _name);
	_events->RemoveListener("ServerSend_Dispose", _name);
	_events->RemoveListener("ServerSend_Shot", _name);
	_events->RemoveListener("ServerSend_Statistics", _name);

	_events->RemoveListener("ServerSend_AnimationCreate", _name);

	_events->RemoveListener("ServerSend_OnTankOnOff", _name);

	UnsubscribeBonus();
}

void Server::UnsubscribeBonus() const
{
	_events->RemoveListener("ServerSend_BonusSpawn", _name);
	_events->RemoveListener("ServerSend_BonusDeSpawn", _name);

	_events->RemoveListener("ServerSend_FortressChange", _name);

	_events->RemoveListener("ServerSend_OnBonusHelmet", _name);
	// _events->RemoveListener("ServerSend_OnStar", _name);//TODO: refactor to SendCommand(std::make_shared<
	// _events->RemoveListener("ServerSend_OnCaliber", _name);//TODO: refactor to SendCommand(std::make_shared<
	// _events->RemoveListener("ServerSend_OnTank", _name);//TODO: refactor to SendCommand(std::make_shared<
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
				//TODO: add feature to restart game with existing session
				_sessions.emplace_back(std::make_shared<Session>(std::move(socket), _events));
				if (const auto& lastSession = _sessions.back(); lastSession)
				{
					// _events->EmitEvent("NewClientConnected");
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

	// NetworkLogger::LogServerSend(command->GetClassNameW());

	const auto& basicString = archiveStream.str();
	// NetworkLogger::WriteLog("\nraw data: " + basicString +" =", true);
	this->SendToAll(basicString + "\n\n");
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

void Server::OnCaliber(const std::string& who) const
{
	ServerData data;
	data.who = who;
	data.eventName = "OnCaliber";

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
