#include "network/Client.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ObstacleAndBonusEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/StatisticsEvents.h"
#include "entities/ObjRectangle.h"
#include "enums/CommandType.h"
#include "enums/StatisticsType.h"
#include "enums/TankType.h"
#include "network/commands/CommandBatch.h"
#include "utils/NetworkLogger.h"
// #include <fstream>
#include <ser20/archives/portable_binary.hpp>
#include <cassert>
#include <iostream>
#include <string>
#include <tuple>

namespace network::commands
{
Client::Client(boost::asio::io_context& ioContext, std::string host, uint16_t port,
			   const std::shared_ptr<EventSystem>& events)
	: _strand(boost::asio::make_strand(ioContext))
	, _socket(_strand)
	, _reconnectTimer(_strand)
	, _endpoint{tcp::endpoint(boost::asio::ip::make_address(host), port)}
	, _events{events}
{
	Subscribe();
	RegisterCommandHandlers();

	TryConnect();
}

void Client::RegisterCommandHandlers()
{
	_commandHandlers = {
			{CommandType::POSITION_CHANGE, [this](const AnyCommand& cmd) { OnPositionChange(cmd); }},
			//TODO: refactored tankShot event to bullet pool spawn with bulletId
			{CommandType::TANK_SHOT, [this](const AnyCommand& cmd) { OnTankShot(cmd); }},
			{CommandType::HEALTH_CHANGE, [this](const AnyCommand& cmd) { OnHealthChange(cmd); }},
			{CommandType::DISPOSE, [this](const AnyCommand& cmd) { OnDispose(cmd); }},
			{CommandType::STATISTICS_CHANGE, [this](const AnyCommand& cmd) { OnStatisticsChange(cmd); }},
			{CommandType::KEY_STATE_CHANGE, [this](const AnyCommand& cmd) { OnKeyStateChange(cmd); }},
			{CommandType::GAME_STATE_CHANGE, [this](const AnyCommand& cmd) { OnGameStateChange(cmd); }},
			{CommandType::FORTRESS_CHANGE, [this](const AnyCommand& cmd) { OnFortressChange(cmd); }},
			{CommandType::BONUS_SPAWN, [this](const AnyCommand& cmd) { OnBonusSpawn(cmd); }},
			{CommandType::BONUS_DESPAWN, [this](const AnyCommand& cmd) { OnBonusDeSpawn(cmd); }},
			{CommandType::RESPAWN_TANK, [this](const AnyCommand& cmd) { OnRespawnTank(cmd); }},
			{CommandType::OBSTACLE_SPAWN, [this](const AnyCommand& cmd) { OnObstacleSpawn(cmd); }},
			{CommandType::TANK_SPAWN_COMPLETE, [this](const AnyCommand& cmd) { OnTankSpawnComplete(cmd); }},
			{CommandType::BONUS_STATUS, [this](const AnyCommand& cmd) { OnBonusStatus(cmd); }},
	};
}

//TODO: fix reconnect for minGW when we too fast run host and client
void Client::TryConnect()
{
	if (_socket.is_open())
	{
		boost::system::error_code ec;
		std::ignore = _socket.close(ec);// NOTE: error captured via ec, return value intentionally discarded
	}

	_socket.open(_endpoint.protocol());
	_socket.async_connect(_endpoint, [this](const boost::system::error_code& ec)
	{
		if (!ec)
		{
			// std::cout << "Client connected successfully" << '\n';
			_reconnectAttempts = 0;
			_isConnected = true;
			this->ReadResponse();
			{
				std::scoped_lock lock(_batchWriteMutex);
				this->_batch.AddCommand(SignalEvent{"ClientOut_ReadyToPlay"});
			}

			TryStartWrite();
		}
		else
		{
			++_reconnectAttempts;
			std::cerr << "Client connect failed (attempt " << _reconnectAttempts
					<< "/" << MaxReconnectAttempts << "): " << ec.message() << '\n';

			if (_reconnectAttempts < MaxReconnectAttempts)
			{
				ScheduleReconnect();
			}
			else
			{
				std::cerr << "Client gave up after " << MaxReconnectAttempts << " attempts" << '\n';
			}
		}
	});
}

void Client::ScheduleReconnect()
{
	if (_reconnectPending || _isShuttingDown)
	{
		return;
	}

	_reconnectPending = true;

	auto self(shared_from_this());
	_reconnectTimer.expires_after(std::chrono::milliseconds(ReconnectDelayMs));
	_reconnectTimer.async_wait([this, self](const boost::system::error_code& timerEc)
	{
		std::ignore = self;

		_reconnectPending = false;
		if (!timerEc && !_isShuttingDown)
		{
			TryConnect();
		}
	});
}

void Client::HandleDisconnect()
{
	if (_isShuttingDown || _reconnectPending)
	{
		return;
	}

	_isConnected = false;
	_writeInProgress = false;//NOTE: the failed write never completes, so nothing else would clear it

	if (_socket.is_open())
	{
		boost::system::error_code ec;
		std::ignore = _socket.close(ec);
	}

	_reconnectAttempts = 0;//NOTE: a drop starts a fresh budget, it is not a failed connect attempt
	ScheduleReconnect();
}

Client::~Client()
{
	Shutdown();
}

void Client::Shutdown()
{
	try
	{
		_isShuttingDown = true;//NOTE: before cancelling - handlers must not read the cancel as a drop
		_reconnectTimer.cancel();

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
				std::cerr << "Error during socket close: " << ec.message() << '\n';
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception in Client::Shutdown: " << e.what() << '\n';
	}
	catch (...)
	{
		std::cerr << "Unknown error in Client::Shutdown" << '\n';
	}
}

void Client::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &Client::OnNetworkEndFrame));

	// NOTE: local dispatch is keyed (MoveUpEvent/"P2") but the wire format sent via SendKeyState is
	// unchanged ("P2_Move_Up" etc.) - Session::OnKeyStateChange on the host still parses that
	// literal tag+action string out of the KeyStateChange command payload.
	_subs.push_back(_events->AddListener(Key(std::string{"P2"}), this, &Client::OnMoveUp));
	_subs.push_back(_events->AddListener(Key(std::string{"P2"}), this, &Client::OnMoveLeft));
	_subs.push_back(_events->AddListener(Key(std::string{"P2"}), this, &Client::OnMoveDown));
	_subs.push_back(_events->AddListener(Key(std::string{"P2"}), this, &Client::OnMoveRight));
	_subs.push_back(_events->AddListener(Key(std::string{"P2"}), this, &Client::OnFire));

	_subs.push_back(_events->AddListener(this, &Client::OnClientOutReadyToPlay));
	_subs.push_back(_events->AddListener(this, &Client::OnClientOutPauseStatus));
}

void Client::OnNetworkEndFrame(const NetworkEndFrameEvent&)
{
	CommandBatch batch;
	{
		std::scoped_lock lock(_batchWriteMutex);
		std::swap(batch, _batch);
	}

	if (!batch.IsEmpty())
	{
		SendCommand(batch);
	}
}

void Client::OnMoveUp(const MoveUpEvent& event) { SendKeyState("P2_Move_Up", event.isPressed); }
void Client::OnMoveLeft(const MoveLeftEvent& event) { SendKeyState("P2_Move_Left", event.isPressed); }
void Client::OnMoveDown(const MoveDownEvent& event) { SendKeyState("P2_Move_Down", event.isPressed); }
void Client::OnMoveRight(const MoveRightEvent& event) { SendKeyState("P2_Move_Right", event.isPressed); }
void Client::OnFire(const FireEvent& event) { SendKeyState("P2_Fire", event.isPressed); }

void Client::OnClientOutReadyToPlay(const ClientOutReadyToPlayEvent&)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(SignalEvent{"ClientOut_ReadyToPlay"});
}

void Client::OnClientOutPauseStatus(const ClientOutPauseStatusEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(KeyStateChange{"Pause_Released", event.isPaused});
}

void Client::ReadResponse()
{
	auto self(shared_from_this());
	auto lambda = [this, self](const boost::system::error_code& ec, const std::size_t /*length*/)
	{
		//NOTE: self is captured only to keep this Client alive for the duration of the async
		//operation (shared_from_this() lifetime extension) - never dereferenced explicitly
		std::ignore = self;

		if (ec)
		{
			if (ec != boost::asio::error::eof && ec != boost::asio::error::operation_aborted)
			{
				std::cerr << ec.message() << '\n';
			}

			HandleDisconnect();
			return;
		}

		const std::uint32_t payloadLength = network::DecodeFrameHeader(_readHeader.data());
		if (payloadLength == 0u || payloadLength > network::kMaxFramePayloadSize)
		{
			NetworkLogger::WriteLog("Client::ReadResponse: bogus frame length "
									+ std::to_string(payloadLength) + ", dropping connection");
			HandleDisconnect();
			return;
		}

		ReadPayload(payloadLength);
	};

	boost::asio::async_read(_socket, boost::asio::buffer(_readHeader), std::move(lambda));
}

void Client::ReadPayload(const std::uint32_t payloadLength)
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
				std::cerr << ec.message() << '\n';
			}

			HandleDisconnect();
			return;
		}

		ProcessReceivedData(std::string(_readPayload.data(), _readPayload.size()));

		// Since we want to keep listening, initiate reading again
		this->ReadResponse();
	};

	boost::asio::async_read(_socket, boost::asio::buffer(_readPayload), std::move(lambda));
}

void Client::SendKeyState(const std::string& key, const bool state)
{
	// NetworkLogger::LogClientOut(state);
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(KeyStateChange{key, state});
}

void Client::OnPositionChange(const AnyCommand& command)
{
	const auto& cmd = std::get<PositionChange>(command);
	const auto pos = cmd.GetPos();
	const auto dir = cmd.GetDir();
	const auto uuid = cmd.GetUuid();

	_commandQueue.Enqueue([this, pos, dir, uuid]()
	{
		_events->EmitEvent(Key(uuid), ClientInPosEvent{.pos = pos, .dir = dir});
	});
}

void Client::OnTankShot(const AnyCommand& command)
{
	const auto& cmd = std::get<TankShot>(command);
	const auto who = cmd.GetWho();
	const auto dir = cmd.GetDir();
	const auto uuid = cmd.GetUuid();

	_commandQueue.Enqueue([this, who, dir, uuid]()
	{
		_events->EmitEvent(Key(who), ClientInShotEvent{.dir = dir, .bulletUuid = uuid});
	});
}

void Client::OnHealthChange(const AnyCommand& command)
{
	const auto& cmd = std::get<HealthChange>(command);
	const auto uuid = cmd.GetUuid();
	const auto health = cmd.GetHealth();

	_commandQueue.Enqueue([this, uuid, health]()
	{
		_events->EmitEvent(Key(uuid), ClientInHealthEvent{.health = health});
	});
}

void Client::OnDispose(const AnyCommand& command)
{
	const auto& cmd = std::get<Dispose>(command);
	const auto uuid = cmd.GetUuid();

	_commandQueue.Enqueue([this, uuid]()
	{
		_events->EmitEvent(Key(uuid), ClientInDisposeEvent{});
	});
}

void Client::OnStatisticsChange(const AnyCommand& command)
{
	const auto& cmd = std::get<StatisticsChange>(command);
	const auto type = cmd.GetStatisticsType();
	const auto author = cmd.GetAuthor();
	const auto fraction = cmd.GetFraction();

	_commandQueue.Enqueue([this, type, author, fraction]()
	{
		switch (type)
		{
			case StatisticsType::BulletHit:
				_events->EmitEvent(ClientInBulletHitEvent{.author = author, .fraction = fraction});
				break;
			case StatisticsType::EnemyHit:
				_events->EmitEvent(ClientInEnemyHitEvent{.author = author, .fraction = fraction});
				break;
			case StatisticsType::PlayerOneHit:
				_events->EmitEvent(ClientInPlayerOneHitEvent{.author = author, .fraction = fraction});
				break;
			case StatisticsType::PlayerTwoHit:
				_events->EmitEvent(ClientInPlayerTwoHitEvent{.author = author, .fraction = fraction});
				break;
			case StatisticsType::EnemyDied:
				_events->EmitEvent(ClientInEnemyDiedEvent{.author = author, .fraction = fraction});
				break;
			case StatisticsType::PlayerOneDied:
				_events->EmitEvent(ClientInPlayerOneDiedEvent{.author = author, .fraction = fraction});
				break;
			case StatisticsType::PlayerTwoDied:
				_events->EmitEvent(ClientInPlayerTwoDiedEvent{.author = author, .fraction = fraction});
				break;
			case StatisticsType::BrickWallDied:
				_events->EmitEvent(ClientInBrickWallDiedEvent{.author = author, .fraction = fraction});
				break;
			case StatisticsType::SteelWallDied:
				_events->EmitEvent(ClientInSteelWallDiedEvent{.author = author, .fraction = fraction});
				break;
			case StatisticsType::BonusPickup:
				_events->EmitEvent(ClientInBonusPickupEvent{.author = author, .fraction = fraction});
				break;
			case StatisticsType::BonusDestroyed:
				_events->EmitEvent(ClientInBonusDestroyedEvent{.author = author, .fraction = fraction});
				break;
		}
	});
}

void Client::OnKeyStateChange(const AnyCommand& command)
{
	const auto& cmd = std::get<KeyStateChange>(command);
	const auto keyState = cmd.GetKeyState();
	const auto isEnable = cmd.GetIsEnable();

	_commandQueue.Enqueue([this, keyState, isEnable]()
	{
		if (keyState == "Pause_Status")
		{
			this->_events->EmitEvent(PauseStatusEvent{.isPaused = isEnable});
		}
		else
		{
			NetworkLogger::WriteLog("Client::OnKeyStateChange: unrecognized key state \"" + keyState + "\"");
		}
	});
}

void Client::OnGameStateChange(const AnyCommand& command)
{
	const auto& cmd = std::get<GameStateChange>(command);
	const auto gameState = cmd.GetGameState();

	_commandQueue.Enqueue([this, gameState]()
	{
		if (gameState == "PlayersTeamIsWon")
		{
			this->_events->EmitEvent(PlayersTeamIsWonEvent{});
		}
		else if (gameState == "EnemiesTeamIsWon")
		{
			this->_events->EmitEvent(EnemiesTeamIsWonEvent{});
		}
		else
		{
			NetworkLogger::WriteLog("Client::OnGameStateChange: unrecognized game state \"" + gameState + "\"");
		}
	});
}

void Client::OnFortressChange(const AnyCommand& command)
{
	const auto& cmd = std::get<FortressChange>(command);
	const std::string state = cmd.GetState();
	const auto uuid = cmd.GetUuid();

	_commandQueue.Enqueue([this, state, uuid]()
	{
		_events->EmitEvent(Key(uuid), ClientInFortressChangeEvent{.state = state});
	});
}

void Client::OnBonusSpawn(const AnyCommand& command)
{
	const auto& cmd = std::get<BonusSpawn>(command);
	const auto pos = cmd.GetPos();
	const auto bonusType = cmd.GetBonusType();
	const auto uuid = cmd.GetUuid();

	_commandQueue.Enqueue([this, pos, bonusType, uuid]()
	{
		_events->EmitEvent(ClientInBonusSpawnEvent{.pos = pos, .type = bonusType, .uuid = uuid});
	});
}

void Client::OnBonusDeSpawn(const AnyCommand& command)
{
	const auto& cmd = std::get<BonusDeSpawn>(command);
	const auto uuid = cmd.GetUuid();

	_commandQueue.Enqueue([this, uuid]()
	{
		_events->EmitEvent(ClientInBonusDeSpawnEvent{.uuid = uuid});
	});
}

void Client::OnRespawnTank(const AnyCommand& command)
{
	const auto& cmd = std::get<RespawnTank>(command);
	const auto tankType = cmd.GetTankType();
	const auto uuid = cmd.GetUuid();
	const auto rect = cmd.GetRect();

	_commandQueue.Enqueue([this, tankType, uuid, rect]()
	{
		_events->EmitEvent(ClientInRespawnTankEvent{.type = tankType, .uuid = uuid, .rect = rect});
	});
}

void Client::OnObstacleSpawn(const AnyCommand& command)
{
	const auto& cmd = std::get<ObstacleSpawn>(command);
	const auto rect = cmd.GetRect();
	const auto obstacleType = cmd.GetObstacleType();
	const auto uuid = cmd.GetUuid();

	_commandQueue.Enqueue([this, rect, obstacleType, uuid]()
	{
		_events->EmitEvent(ClientInObstacleSpawnEvent{.rect = rect, .type = obstacleType, .uuid = uuid});
	});
}

void Client::OnTankSpawnComplete(const AnyCommand& command)
{
	const auto& cmd = std::get<TankSpawnComplete>(command);
	const auto uuid = cmd.GetUuid();

	_commandQueue.Enqueue([this, uuid]()
	{
		_events->EmitEvent(ClientInTankSpawnCompleteEvent{.uuid = uuid});
	});
}

void Client::OnBonusStatus(const AnyCommand& command)
{
	const auto& cmd = std::get<BonusStatus>(command);
	const auto bonusType = cmd.GetBonusType();
	const auto name = cmd.GetName();
	const auto isEnable = cmd.GetIsEnable();

	_commandQueue.Enqueue([this, bonusType, name, isEnable]()
	{
		switch (bonusType)
		{
			case BonusType::Helmet:
				_events->EmitEvent(Key(name), ClientInBonusHelmetPickupEvent{.isEnable = isEnable});
				break;
			case BonusType::Star:
				_events->EmitEvent(Key(name), ClientInBonusStarPickupEvent{});
				break;
			case BonusType::Caliber:
				_events->EmitEvent(Key(name), ClientInBonusCaliberPickupEvent{});
				break;
			case BonusType::Tank:
				_events->EmitEvent(ClientInBonusTankPickupEvent{.name = name});
				break;
			default:
				//NOTE: Server only ever constructs BonusStatus with Helmet/Star/Caliber/Tank
				//(see Server.cpp), so reaching here means a new BonusType wasn't wired up above
				assert(false && "Client::OnBonusStatus: unhandled BonusType");
				break;
		}
	});
}

void Client::ProcessClientCommand(const AnyCommand& command)
{
	// auto commandName = std::string("client receive:") + GetClassNameW(command);
	// NetworkLogger::LogClientIn(commandName);
	if (const auto it = _commandHandlers.find(GetCommandType(command)); it != _commandHandlers.end())
	{
		it->second(command);
	}
}

void Client::ProcessReceivedData(const std::string& archiveData)
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
			ProcessClientCommand(command);
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

void Client::SendCommand(const CommandBatch& command)
{
	std::ostringstream archiveStream;
	{
		ser20::PortableBinaryOutputArchive oa(archiveStream);
		oa(command);
	}

	//NOTE: called from the game thread - the post is what moves the queue access onto the strand
	auto self(shared_from_this());
	boost::asio::post(_socket.get_executor(),
					  [this, self,
						  framed = std::make_shared<const std::string>(
								  network::FrameMessage(archiveStream.str()))]() mutable
					  {
						  std::ignore = self;

						  if (_writeQueue.size() >= MaxPendingFrames)
						  {
							  _writeQueue.pop_front();
							  NetworkLogger::WriteLog(
									  "Client::SendCommand: pending queue full, dropped oldest frame");
						  }

						  _writeQueue.push_back(std::move(framed));

						  TryStartWrite();
					  });
}

//NOTE: nothing reaches the wire until connected - frames pile up and the connect handler kicks them off
void Client::TryStartWrite()
{
	if (_writeQueue.empty() || !_isConnected || _writeInProgress)
	{
		return;
	}

	_writeInProgress = true;

	WriteNextFrame();
}

void Client::WriteNextFrame()
{
	//NOTE: shared, so the queue stays free to change while the write is in flight
	const auto frame = _writeQueue.front();

	auto self(shared_from_this());
	auto lambda = [this, self, frame](const boost::system::error_code& ec, const std::size_t /*length*/)
	{
		//NOTE: self and frame are captured to outlive the async operation, never dereferenced here
		std::ignore = self;
		std::ignore = frame;

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

			//NOTE: undelivered frame stays at the head, to be re-sent whole on the next connection
			HandleDisconnect();
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
}//namespace network::commands
