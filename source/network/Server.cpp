#include "network/Server.h"
#include "components/EventSystem.h"
#include "components/SpawnEvents.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ObstacleAndBonusEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/StatisticsEvents.h"
#include "entities/ObjRectangle.h"
#include "enums/TankType.h"
#include "network/commands/BonusDeSpawn.h"
#include "network/commands/BonusSpawn.h"
#include "network/commands/BonusStatus.h"
#include "network/commands/CommandBatch.h"
#include "network/commands/Dispose.h"
#include "network/commands/FortressChange.h"
#include "network/commands/GameStateChange.h"
#include "network/commands/HealthChange.h"
#include "network/commands/KeyStateChange.h"
#include "network/commands/ObstacleSpawn.h"
#include "network/commands/PositionChange.h"
#include "network/commands/RespawnTank.h"
#include "network/commands/SignalEvent.h"
#include "network/commands/StatisticsChange.h"
#include "network/commands/TankSpawnComplete.h"
#include "network/commands/TankShot.h"
#include "utils/NetworkLogger.h"
#include <algorithm>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/uuid/uuid.hpp>
#include <fstream>
#include <iostream>
#include <mutex>

// std::ofstream error_log("error_log.txt");
namespace network::commands
{
Session::Session(tcp::socket sock, const std::shared_ptr<EventSystem>& events)
	: _socket(std::move(sock))
	, _events(events)
{
	RegisterCommandHandlers();
}

void Session::RegisterCommandHandlers()
{
	_commandHandlers = {
			{CommandType::COMMAND_BATCH, [this](const std::shared_ptr<Command>& cmd) { OnCommandBatch(cmd); }},
			{CommandType::SIGNAL_EVENT, [this](const std::shared_ptr<Command>& cmd) { OnSignalEvent(cmd); }},
			{CommandType::KEY_STATE_CHANGE, [this](const std::shared_ptr<Command>& cmd) { OnKeyStateChange(cmd); }},
	};
}

Session::~Session()
{
	Shutdown();
}

void Session::Shutdown()
{
	try
	{
		if (_socket.is_open())
		{
			boost::system::error_code ec;
			std::ignore = _socket.cancel(ec);

			std::ignore = _socket.shutdown(tcp::socket::shutdown_both, ec);
			if (ec)
			{
				std::cerr << "Error during socket shutdown: " << ec.message() << '\n';
			}

			std::ignore = _socket.close(ec);
			if (ec)
			{
				std::cerr << "Error closing socket socket: " << ec.message() << '\n';
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception in Session::Shutdown: " << e.what() << '\n';
	}
	catch (...)
	{
		std::cerr << "Unknown error in Session::Shutdown" << '\n';
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

void Session::ProcessReceivedData(const std::string& archiveData)
{
	try
	{
		std::istringstream archiveStream(archiveData);
		boost::archive::text_iarchive ia(archiveStream);

		std::shared_ptr<Command> command;
		ia >> command;

		// NetworkLogger::WriteLog("\nraw data: " + archiveData+" =", true);
		ProcessServerCommand(command);
	}
	catch (const std::exception& e)
	{
		const std::string errorMsg = std::string("error deserialization: ") + e.what();
		NetworkLogger::WriteLog(errorMsg);

		if (archiveData.length() < 200)
		{
			NetworkLogger::WriteLog("raw data: " + archiveData);
		}
		else
		{
			NetworkLogger::WriteLog("raw data (first 200 sym): " + archiveData.substr(0, 200) + "...");
		}

		std::cerr << "Deserialization error: " << e.what() << '\n';
		std::cerr << "Raw data size: " << archiveData.length() << " bytes" << '\n';
	}
}

void Session::OnCommandBatch(const std::shared_ptr<Command>& commands)
{
	if (const auto* cmd = dynamic_cast<CommandBatch*>(commands.get()))
	{
		for (const auto& command: cmd->GetCommands())
		{
			ProcessServerCommand(command);
		}
	}
}

void Session::OnSignalEvent(const std::shared_ptr<Command>& command)
{
	if (const auto* cmd = dynamic_cast<SignalEvent*>(command.get()))
	{
		const std::string signalName = cmd->GetSignalName();

		_commandQueue.Enqueue([this, signalName]()
		{
			if (signalName == "ClientOut_ReadyToPlay")
			{
				_events->EmitEvent(ServerInClientReadyToStartGameEvent{});
			}
			else
			{
				NetworkLogger::WriteLog("Session::OnSignalEvent: unrecognized signal \"" + signalName + "\"");
			}
		});
	}
}

void Session::OnKeyStateChange(const std::shared_ptr<Command>& command)
{
	if (const auto* cmd = dynamic_cast<KeyStateChange*>(command.get()))
	{
		const auto keyState = cmd->GetKeyState();
		const auto isEnable = cmd->GetIsEnable();

		_commandQueue.Enqueue([this, keyState, isEnable]()//TODO: validate each command, security risk
		{
			if (keyState.starts_with("P1_") || keyState.starts_with("P2_"))
			{
				const std::string tag = keyState.substr(0, 2);
				const std::string action = keyState.substr(3);

				if (action == "Move_Up")
				{
					_events->EmitEvent(Key(tag), ServerInMoveUpEvent{.isPressed = isEnable});
				}
				else if (action == "Move_Down")
				{
					_events->EmitEvent(Key(tag), ServerInMoveDownEvent{.isPressed = isEnable});
				}
				else if (action == "Move_Left")
				{
					_events->EmitEvent(Key(tag), ServerInMoveLeftEvent{.isPressed = isEnable});
				}
				else if (action == "Move_Right")
				{
					_events->EmitEvent(Key(tag), ServerInMoveRightEvent{.isPressed = isEnable});
				}
				else if (action == "Fire")
				{
					_events->EmitEvent(Key(tag), ServerInFireEvent{.isPressed = isEnable});
				}
				else
				{
					NetworkLogger::WriteLog("Session::OnKeyStateChange: unrecognized tagged action \"" + action
											+ "\"");
				}
			}
			else if (keyState == "Pause_Released")
			{
				_events->EmitEvent(ServerInPauseReleasedEvent{.isPaused = isEnable});
			}
			else
			{
				NetworkLogger::WriteLog("Session::OnKeyStateChange: unrecognized key state \"" + keyState + "\"");
			}
		});
	}
}

void Session::ProcessServerCommand(const std::shared_ptr<Command>& command)
{
	if (!command)
	{
		return;
	}

	// auto classNameW = std::string(command->GetClassNameW());
	// auto commandName = std::string("client receive:" + classNameW);
	// NetworkLogger::LogClientIn(commandName);
	if (const auto it = _commandHandlers.find(command->GetType()); it != _commandHandlers.end())
	{
		it->second(command);
	}
}

void Session::DoRead()
{
	try
	{
		auto self(shared_from_this());
		auto lambda = [this, self](const boost::system::error_code& ec, const std::size_t length)
		{
			//NOTE: self is captured only to keep this Session alive for the duration of the async
			//operation (shared_from_this() lifetime extension) - never dereferenced explicitly
			std::ignore = self;

			if (ec)
			{
				_readBuffer.consume(length);

				if (ec != boost::asio::error::eof && ec != boost::asio::error::operation_aborted)
				{
					std::cerr << "DoRead error ..." << ec << '\n';
				}
			}
			else
			{
				const std::string archiveData(buffers_begin(_readBuffer.data()),
											  buffers_begin(_readBuffer.data()) + length);
				std::istringstream archiveStream(archiveData);
				boost::archive::text_iarchive ia(archiveStream);

				this->ProcessReceivedData(archiveData);

				// Respond back to a client
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
			std::cerr << "Session Socket is not open. Cannot write.";
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
					std::cout << "Session Connection closed normally" << '\n';
				}
				else
				{
					std::cerr << "Session Write error: " << ec.message() << '\n';
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
		std::cerr << "Session Exception in DoWrite: " << e.what() << '\n';
		NetworkLogger::WriteLog(std::string("Session Exception in DoWrite: ") + e.what());
	}
	catch (...)
	{
		std::cerr << "Session error ..." << '\n';
		NetworkLogger::WriteLog("Session error in DoWrite: unknown exception");
	}
}

Server::Server(boost::asio::io_context& ioContext, std::string host, uint16_t port,
			   const std::shared_ptr<EventSystem>& events)
	: _acceptor{tcp::acceptor(ioContext, tcp::endpoint(boost::asio::ip::make_address(host), port))}
	, _events{events}
	, _name{"Server"}
	, _batch{std::make_shared<CommandBatch>()}
{
	DoAccept();
	StartSendThread();
	Subscribe();
}

//TODO: check the flow after network game choose local mode to clean all then to choose network again successful
void Server::StartSendThread()
{
	_isRunning.store(true);
	_sendThread = std::thread([this]()
	{
		while (this->_isRunning.load())
		{
			std::shared_ptr<CommandBatch> batch;
			{
				std::unique_lock<std::mutex> lock(this->_sendQueueMutex);
				this->_sendCondition.wait(
						lock, [this] { return !this->_sendQueue.empty() || !this->_isRunning.load(); });

				if (!this->_isRunning.load())
					break;

				if (this->_sendQueue.empty())
					continue;

				batch = this->_sendQueue.front();
				this->_sendQueue.pop();
			}

			if (batch && !batch->IsEmpty())
			{
				try
				{
					SendCommand(batch);
				}
				catch (const std::exception& e)
				{
					std::cerr << "Server Exception in send thread: " << e.what() << '\n';

					// retry send
					std::scoped_lock lock(this->_sendQueueMutex);
					this->_sendQueue.push(batch);
				}
				catch (...)
				{
					std::cerr << "Server thread error ..." << '\n';
					NetworkLogger::WriteLog("Server send thread error: unknown exception");
				}
			}
		}
	});
}

void Server::StopSendThread()
{
	{
		std::scoped_lock lock(_sendQueueMutex);
		_isRunning.store(false);
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
	Shutdown();
}

void Server::Shutdown()
{
	if (_acceptor.is_open())
	{
		boost::system::error_code ec;
		std::ignore = _acceptor.cancel(ec);
		std::ignore = _acceptor.close(ec);
	}

	for (const auto& session: _sessions)
	{
		if (session)
		{
			session->Shutdown();
		}
	}
}

void Server::Subscribe()
{
	_subs.push_back(_events->AddListener(_name, [this](const NetworkEndFrameEvent&)
	{
		auto batch{std::make_shared<CommandBatch>()};
		{
			std::scoped_lock lock(_batchWriteMutex);
			std::swap(batch, _batch);
		}
		{
			std::scoped_lock lock(_sendQueueMutex);
			_sendQueue.emplace(batch);
		}
		_sendCondition.notify_one();
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutPauseStatusEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<KeyStateChange>("Pause_Status", event.isPaused));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutPlayersTeamIsWonEvent&)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<GameStateChange>("PlayersTeamIsWon"));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutEnemiesTeamIsWonEvent&)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<GameStateChange>("EnemiesTeamIsWon"));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutPosEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<PositionChange>(event.who, event.pos, event.dir, event.uuid));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutShotEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<TankShot>(event.who, event.dir, event.bulletUuid));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutHealthEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<HealthChange>(event.who, event.health, event.uuid));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutDisposeEvent& event)//TODO: add who,
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<Dispose>("Bullet", event.uuid));
	}));

	SubscribeStatistics();

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutRespawnTankEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<RespawnTank>(event.type, event.uuid, event.rect));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutObstacleSpawnEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<ObstacleSpawn>(event.rect, event.type, event.uuid));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutTankSpawnCompleteEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<TankSpawnComplete>(event.uuid));
	}));

	SubscribeBonus();
}

//NOTE: GameStatistics.cpp emits one of these 11 distinct types directly. StatisticsChange's own
//discriminator is StatisticsType (see enums/StatisticsType.h), not a free-form string, so each
//listener here just names its enum value.
void Server::SubscribeStatistics()
{
	_subs.push_back(_events->AddListener(_name, [this](const ServerOutBulletHitEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<StatisticsChange>(StatisticsType::BulletHit, event.author, event.fraction));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutEnemyHitEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<StatisticsChange>(StatisticsType::EnemyHit, event.author, event.fraction));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutPlayerOneHitEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(
				std::make_shared<StatisticsChange>(StatisticsType::PlayerOneHit, event.author, event.fraction));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutPlayerTwoHitEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(
				std::make_shared<StatisticsChange>(StatisticsType::PlayerTwoHit, event.author, event.fraction));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutEnemyDiedEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<StatisticsChange>(StatisticsType::EnemyDied, event.author, event.fraction));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutPlayerOneDiedEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(
				std::make_shared<StatisticsChange>(StatisticsType::PlayerOneDied, event.author, event.fraction));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutPlayerTwoDiedEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(
				std::make_shared<StatisticsChange>(StatisticsType::PlayerTwoDied, event.author, event.fraction));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutBrickWallDiedEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(
				std::make_shared<StatisticsChange>(StatisticsType::BrickWallDied, event.author, event.fraction));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutSteelWallDiedEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(
				std::make_shared<StatisticsChange>(StatisticsType::SteelWallDied, event.author, event.fraction));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutBonusPickupEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(
				std::make_shared<StatisticsChange>(StatisticsType::BonusPickup, event.author, event.fraction));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutBonusDestroyedEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(
				std::make_shared<StatisticsChange>(StatisticsType::BonusDestroyed, event.author, event.fraction));
	}));
}

void Server::SubscribeBonus()
{
	_subs.push_back(_events->AddListener(
			_name,
			[this](const ServerOutBonusSpawnEvent& event)
			{
				std::scoped_lock lock(_batchWriteMutex);
				_batch->AddCommand(std::make_shared<BonusSpawn>(event.pos, event.type, event.uuid));
			}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutBonusDeSpawnEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<BonusDeSpawn>(event.uuid));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutFortressChangeEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<FortressChange>(event.state, event.uuid));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutBonusHelmetPickupEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<BonusStatus>(event.name, BonusType::Helmet, event.isActive));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutBonusStarPickupEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<BonusStatus>(event.author, BonusType::Star));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutBonusCaliberPickupEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<BonusStatus>(event.author, BonusType::Caliber));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ServerOutBonusTankPickupEvent& event)
	{
		std::scoped_lock lock(_batchWriteMutex);
		_batch->AddCommand(std::make_shared<BonusStatus>(event.author, BonusType::Tank));
	}));
}

void Server::DoAccept()
{
	_acceptor.async_accept([this](const boost::system::error_code& ec, tcp::socket socket)
	{
		if (ec)
		{
			if (ec != boost::asio::error::operation_aborted)
			{
				std::cerr << "Accept error: " << ec.message() << '\n';
			}
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

void Server::CleanupDeadSessions()
{
	std::erase_if(_sessions, [](const std::shared_ptr<Session>& session)
	{
		return !session || !session->IsSocketOpen();
	});
}

void Server::SendToAll(const std::string& message)
{
	CleanupDeadSessions();

	for (const auto& session: _sessions)
	{
		if (session && session->IsSocketOpen())
		{
			session->DoWrite(message);
		}
	}
}

void Server::SendCommand(const std::shared_ptr<Command>& command)
{
	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << command;

	// NetworkLogger::LogServerOut(command->GetClassNameW());

	const auto& basicString = archiveStream.str();
	// NetworkLogger::WriteLog("\nraw data: " + basicString +" =", true);
	SendToAll(basicString + "\n\n");
}
}//namespace network::commands
