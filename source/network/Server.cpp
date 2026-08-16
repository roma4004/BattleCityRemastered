#include "network/Server.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ObstacleAndBonusEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/StatisticsEvents.h"
#include "network/commands/CommandBatch.h"
#include "utils/NetworkLogger.h"
#include <algorithm>
#include <ser20/archives/portable_binary.hpp>
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
			{CommandType::SIGNAL_EVENT, [this](const AnyCommand& cmd) { OnSignalEvent(cmd); }},
			{CommandType::KEY_STATE_CHANGE, [this](const AnyCommand& cmd) { OnKeyStateChange(cmd); }},
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
		ser20::PortableBinaryInputArchive ia(archiveStream);

		CommandBatch batch;
		ia(batch);

		// NetworkLogger::WriteLog("\nraw data: " + archiveData+" =", true);
		for (const auto& command: batch.GetCommands())
		{
			ProcessServerCommand(command);
		}
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

void Session::OnSignalEvent(const AnyCommand& command)
{
	const auto& cmd = std::get<SignalEvent>(command);
	const std::string signalName = cmd.GetSignalName();

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

void Session::OnKeyStateChange(const AnyCommand& command)
{
	const auto& cmd = std::get<KeyStateChange>(command);
	const auto keyState = cmd.GetKeyState();
	const auto isEnable = cmd.GetIsEnable();

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

void Session::ProcessServerCommand(const AnyCommand& command)
{
	// auto commandName = std::string("client receive:") + GetClassNameW(command);
	// NetworkLogger::LogClientIn(commandName);
	if (const auto it = _commandHandlers.find(GetCommandType(command)); it != _commandHandlers.end())
	{
		it->second(command);
	}
}

void Session::DoRead()
{
	try
	{
		auto self(shared_from_this());
		auto lambda = [this, self](const boost::system::error_code& ec, const std::size_t /*length*/)
		{
			//NOTE: self is captured only to keep this Session alive for the duration of the async
			//operation (shared_from_this() lifetime extension) - never dereferenced explicitly
			std::ignore = self;

			if (ec)
			{
				if (ec != boost::asio::error::eof && ec != boost::asio::error::operation_aborted)
				{
					std::cerr << "DoRead error ..." << ec << '\n';
				}

				Shutdown();//NOTE: CleanupDeadSessions goes by IsSocketOpen(), so it must be closed here
				return;
			}

			const std::uint32_t payloadLength = network::DecodeFrameHeader(_readHeader.data());
			if (payloadLength == 0u || payloadLength > network::kMaxFramePayloadSize)
			{
				NetworkLogger::WriteLog("Session::DoRead: bogus frame length "
										+ std::to_string(payloadLength) + ", dropping connection");
				boost::system::error_code closeEc;
				std::ignore = _socket.close(closeEc);
				return;
			}

			ReadPayload(payloadLength);
		};

		boost::asio::async_read(_socket, boost::asio::buffer(_readHeader), lambda);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception in DoRead: " << e.what() << '\n';
	}
	catch (...)
	{
		std::cerr << "error ..." << '\n';
	}
}

void Session::ReadPayload(const std::uint32_t payloadLength)
{
	_readPayload.resize(payloadLength);

	auto self(shared_from_this());
	auto lambda = [this, self](const boost::system::error_code& ec, const std::size_t /*length*/)
	{
		std::ignore = self;

		if (ec)
		{
			if (ec != boost::asio::error::eof && ec != boost::asio::error::operation_aborted)
			{
				std::cerr << "ReadPayload error ..." << ec << '\n';
			}

			Shutdown();
			return;
		}

		this->ProcessReceivedData(std::string(_readPayload.data(), _readPayload.size()));

		DoRead();
	};

	boost::asio::async_read(_socket, boost::asio::buffer(_readPayload), lambda);
}

void Session::DoWrite(std::shared_ptr<const std::string> message)
{
	try
	{
		if (!_socket.is_open())
		{
			std::cerr << "Session Socket is not open. Cannot write.";
			return;
		}

		//NOTE: called from the send thread - the post is what moves the queue access onto the strand
		auto self(shared_from_this());
		boost::asio::post(_socket.get_executor(), [this, self, message = std::move(message)]() mutable
		{
			std::ignore = self;

			_writeQueue.push_back(std::move(message));

			TryStartWrite();
		});
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

void Session::TryStartWrite()
{
	if (_writeQueue.empty() || _writeInProgress)
	{
		return;
	}

	_writeInProgress = true;

	WriteNextFrame();
}

void Session::WriteNextFrame()
{
	//NOTE: shared, so the queue stays free to change while the write is in flight
	const auto frame = _writeQueue.front();

	auto self(shared_from_this());
	auto lambda = [this, self, frame](const boost::system::error_code& ec, const std::size_t /*length*/)
	{
		std::ignore = self;
		std::ignore = frame;

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

			//NOTE: undelivered frame stays queued; the session is discarded whole once the socket closes
			_writeInProgress = false;
			_socket.close();
			return;
		}

		_writeQueue.pop_front();

		if (_writeQueue.empty())
		{
			_writeInProgress = false;
			return;
		}

		WriteNextFrame();
	};

	boost::asio::async_write(_socket, boost::asio::buffer(*frame), std::move(lambda));
}

Server::Server(boost::asio::io_context& ioContext, std::string host, uint16_t port,
			   const std::shared_ptr<EventSystem>& events)
	: _acceptor{tcp::acceptor(ioContext, tcp::endpoint(boost::asio::ip::make_address(host), port))}
	, _events{events}
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
			CommandBatch batch;
			{
				std::unique_lock<std::mutex> lock(this->_sendQueueMutex);
				this->_sendCondition.wait(
						lock, [this] { return !this->_sendQueue.empty() || !this->_isRunning.load(); });

				if (!this->_isRunning.load())
					break;

				if (this->_sendQueue.empty())
					continue;

				batch = std::move(this->_sendQueue.front());
				this->_sendQueue.pop();
			}

			if (!batch.IsEmpty())
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
					this->_sendQueue.push(std::move(batch));
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

	for (const auto& session: SnapshotSessions())
	{
		if (session)
		{
			session->Shutdown();
		}
	}
}

void Server::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &Server::OnNetworkEndFrame));
	_subs.push_back(_events->AddListener(this, &Server::OnPauseStatus));
	_subs.push_back(_events->AddListener(this, &Server::OnPlayersTeamIsWon));
	_subs.push_back(_events->AddListener(this, &Server::OnEnemiesTeamIsWon));
	_subs.push_back(_events->AddListener(this, &Server::OnPos));
	_subs.push_back(_events->AddListener(this, &Server::OnShot));
	_subs.push_back(_events->AddListener(this, &Server::OnHealth));
	_subs.push_back(_events->AddListener(this, &Server::OnDispose));

	SubscribeStatistics();

	_subs.push_back(_events->AddListener(this, &Server::OnRespawnTank));
	_subs.push_back(_events->AddListener(this, &Server::OnObstacleSpawn));
	_subs.push_back(_events->AddListener(this, &Server::OnTankSpawnComplete));

	SubscribeBonus();
}

void Server::OnNetworkEndFrame(const NetworkEndFrameEvent&)
{
	CommandBatch batch;
	{
		std::scoped_lock lock(_batchWriteMutex);
		std::swap(batch, _batch);
	}
	{
		std::scoped_lock lock(_sendQueueMutex);
		_sendQueue.push(std::move(batch));
	}
	_sendCondition.notify_one();
}

void Server::OnPauseStatus(const ServerOutPauseStatusEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(KeyStateChange{"Pause_Status", event.isPaused});
}

void Server::OnPlayersTeamIsWon(const ServerOutPlayersTeamIsWonEvent&)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(GameStateChange{"PlayersTeamIsWon"});
}

void Server::OnEnemiesTeamIsWon(const ServerOutEnemiesTeamIsWonEvent&)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(GameStateChange{"EnemiesTeamIsWon"});
}

void Server::OnPos(const ServerOutPosEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(PositionChange{event.who, event.pos, event.dir, event.uuid});
}

void Server::OnShot(const ServerOutShotEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(TankShot{event.who, event.dir, event.bulletUuid});
}

void Server::OnHealth(const ServerOutHealthEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(HealthChange{event.who, event.health, event.uuid});
}

void Server::OnDispose(const ServerOutDisposeEvent& event)//TODO: add who,
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(Dispose{"Bullet", event.uuid});
}

void Server::OnRespawnTank(const ServerOutRespawnTankEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(RespawnTank{event.type, event.uuid, event.rect});
}

void Server::OnObstacleSpawn(const ServerOutObstacleSpawnEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(ObstacleSpawn{event.rect, event.type, event.uuid});
}

void Server::OnTankSpawnComplete(const ServerOutTankSpawnCompleteEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(TankSpawnComplete{event.uuid});
}

//NOTE: GameStatistics.cpp emits one of these 11 distinct types directly. StatisticsChange's own
//discriminator is StatisticsType (see enums/StatisticsType.h), not a free-form string, so each
//listener here just names its enum value.
void Server::SubscribeStatistics()
{
	_subs.push_back(_events->AddListener(this, &Server::OnBulletHit));
	_subs.push_back(_events->AddListener(this, &Server::OnEnemyHit));
	_subs.push_back(_events->AddListener(this, &Server::OnPlayerOneHit));
	_subs.push_back(_events->AddListener(this, &Server::OnPlayerTwoHit));
	_subs.push_back(_events->AddListener(this, &Server::OnEnemyDied));
	_subs.push_back(_events->AddListener(this, &Server::OnPlayerOneDied));
	_subs.push_back(_events->AddListener(this, &Server::OnPlayerTwoDied));
	_subs.push_back(_events->AddListener(this, &Server::OnBrickWallDied));
	_subs.push_back(_events->AddListener(this, &Server::OnSteelWallDied));
	_subs.push_back(_events->AddListener(this, &Server::OnBonusPickup));
	_subs.push_back(_events->AddListener(this, &Server::OnBonusDestroyed));
}

void Server::OnBulletHit(const ServerOutBulletHitEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(StatisticsChange{StatisticsType::BulletHit, event.author, event.fraction});
}

void Server::OnEnemyHit(const ServerOutEnemyHitEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(StatisticsChange{StatisticsType::EnemyHit, event.author, event.fraction});
}

void Server::OnPlayerOneHit(const ServerOutPlayerOneHitEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(StatisticsChange{StatisticsType::PlayerOneHit, event.author, event.fraction});
}

void Server::OnPlayerTwoHit(const ServerOutPlayerTwoHitEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(StatisticsChange{StatisticsType::PlayerTwoHit, event.author, event.fraction});
}

void Server::OnEnemyDied(const ServerOutEnemyDiedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(StatisticsChange{StatisticsType::EnemyDied, event.author, event.fraction});
}

void Server::OnPlayerOneDied(const ServerOutPlayerOneDiedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(StatisticsChange{StatisticsType::PlayerOneDied, event.author, event.fraction});
}

void Server::OnPlayerTwoDied(const ServerOutPlayerTwoDiedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(StatisticsChange{StatisticsType::PlayerTwoDied, event.author, event.fraction});
}

void Server::OnBrickWallDied(const ServerOutBrickWallDiedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(StatisticsChange{StatisticsType::BrickWallDied, event.author, event.fraction});
}

void Server::OnSteelWallDied(const ServerOutSteelWallDiedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(StatisticsChange{StatisticsType::SteelWallDied, event.author, event.fraction});
}

void Server::OnBonusPickup(const ServerOutBonusPickupEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(StatisticsChange{StatisticsType::BonusPickup, event.author, event.fraction});
}

void Server::OnBonusDestroyed(const ServerOutBonusDestroyedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(StatisticsChange{StatisticsType::BonusDestroyed, event.author, event.fraction});
}

void Server::SubscribeBonus()
{
	_subs.push_back(_events->AddListener(this, &Server::OnBonusSpawn));
	_subs.push_back(_events->AddListener(this, &Server::OnBonusDeSpawn));
	_subs.push_back(_events->AddListener(this, &Server::OnFortressChange));
	_subs.push_back(_events->AddListener(this, &Server::OnBonusHelmetPickup));
	_subs.push_back(_events->AddListener(this, &Server::OnBonusStarPickup));
	_subs.push_back(_events->AddListener(this, &Server::OnBonusCaliberPickup));
	_subs.push_back(_events->AddListener(this, &Server::OnBonusTankPickup));
}

void Server::OnBonusSpawn(const ServerOutBonusSpawnEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(BonusSpawn{event.pos, event.type, event.uuid});
}

void Server::OnBonusDeSpawn(const ServerOutBonusDeSpawnEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(BonusDeSpawn{event.uuid});
}

void Server::OnFortressChange(const ServerOutFortressChangeEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(FortressChange{event.state, event.uuid});
}

void Server::OnBonusHelmetPickup(const ServerOutBonusHelmetPickupEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(BonusStatus{event.name, BonusType::Helmet, event.isActive});
}

void Server::OnBonusStarPickup(const ServerOutBonusStarPickupEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(BonusStatus{event.author, BonusType::Star});
}

void Server::OnBonusCaliberPickup(const ServerOutBonusCaliberPickupEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(BonusStatus{event.author, BonusType::Caliber});
}

void Server::OnBonusTankPickup(const ServerOutBonusTankPickupEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(BonusStatus{event.author, BonusType::Tank});
}

void Server::DoAccept()
{
	//NOTE: own strand per socket - serialises that session's handlers against each other
	_acceptor.async_accept(boost::asio::make_strand(_acceptor.get_executor()),
						   [this](const boost::system::error_code& ec, tcp::socket socket)
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
				auto session = std::make_shared<Session>(std::move(socket), _events);
				{
					std::scoped_lock lock(_sessionsMutex);
					_sessions.emplace_back(session);
				}
				// _events->EmitEvent("NewClientConnected");
				session->Start();//NOTE: outside the lock - it posts reads and can reach the event bus
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

std::vector<std::shared_ptr<Session>> Server::SnapshotSessions() const
{
	std::scoped_lock lock(_sessionsMutex);
	return _sessions;
}

void Server::CleanupDeadSessions()
{
	std::vector<std::shared_ptr<Session>> dead;//NOTE: ~Session closes a socket - not under the lock
	{
		std::scoped_lock lock(_sessionsMutex);
		auto removed = std::ranges::remove_if(_sessions, [](const std::shared_ptr<Session>& session)
		{
			return !session || !session->IsSocketOpen();
		});
		dead.insert(dead.end(), std::make_move_iterator(removed.begin()), std::make_move_iterator(removed.end()));
		_sessions.erase(removed.begin(), removed.end());
	}
}

void Server::SendToAll(const std::shared_ptr<const std::string>& message)
{
	CleanupDeadSessions();

	for (const auto& session: SnapshotSessions())
	{
		if (session && session->IsSocketOpen())
		{
			session->DoWrite(message);
		}
	}
}

void Server::ProcessNetworkCommands() const
{
	for (const auto& session: SnapshotSessions())
	{
		if (session && session->IsSocketOpen())
		{
			session->GetCommandQueue().ProcessAll();//TODO: refactor to session->ProcessCommandQueue()
		}
	}
}

void Server::SendCommand(const CommandBatch& command)
{
	std::ostringstream archiveStream;
	{
		ser20::PortableBinaryOutputArchive oa(archiveStream);
		oa(command);
	}

	// NetworkLogger::LogServerOut(command.GetClassNamesW());

	const auto& basicString = archiveStream.str();
	// NetworkLogger::WriteLog("\nraw data: " + basicString +" =", true);
	SendToAll(std::make_shared<const std::string>(network::FrameMessage(basicString)));
}
}//namespace network::commands
