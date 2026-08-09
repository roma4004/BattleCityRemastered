#include "network/Client.h"
#include "components/EventSystem.h"
#include "components/SpawnEvents.h"
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
#include "network/commands/BonusDeSpawn.h"
#include "network/commands/BonusSpawn.h"
#include "network/commands/Command.h"
#include "network/commands/CommandBatch.h"
#include "network/commands/Dispose.h"
#include "network/commands/FortressChange.h"
#include "network/commands/HealthChange.h"
#include "network/commands/KeyStateChange.h"
#include "network/commands/ObstacleSpawn.h"
#include "network/commands/PositionChange.h"
#include "network/commands/RespawnTank.h"
#include "network/commands/StatisticsChange.h"
#include "network/commands/TankShot.h"
#include "utils/NetworkLogger.h"
// #include <fstream>
#include "enums/AnimationType.h"
#include "network/commands/AnimationCreate.h"
#include "network/commands/BonusStatus.h"
#include "network/commands/GameStateChange.h"
#include "network/commands/SignalEvent.h"
#include "network/commands/TankOnOff.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <cassert>
#include <iostream>
#include <string>
#include <tuple>

namespace network::commands
{
Client::Client(boost::asio::io_context& ioContext, std::string host, uint16_t port,
			   const std::shared_ptr<EventSystem>& events)
	: _socket(ioContext)
	, _reconnectTimer(ioContext)
	, _endpoint{tcp::endpoint(boost::asio::ip::make_address(host), port)}
	, _events{events}
	, _name{"Client"}
	, _batch{std::make_shared<CommandBatch>()}
{
	Subscribe();

	TryConnect();
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
				this->_batch->AddCommand(std::make_shared<SignalEvent>("ClientOut_ReadyToPlay"));
			}
		}
		else
		{
			++_reconnectAttempts;
			std::cerr << "Client connect failed (attempt " << _reconnectAttempts
					<< "/" << MaxReconnectAttempts << "): " << ec.message() << '\n';

			if (_reconnectAttempts < MaxReconnectAttempts)
			{
				_reconnectTimer.expires_after(std::chrono::milliseconds(ReconnectDelayMs));
				_reconnectTimer.async_wait([this](const boost::system::error_code& timerEc)
				{
					if (!timerEc)
					{
						TryConnect();
					}
				});
			}
			else
			{
				std::cerr << "Client gave up after " << MaxReconnectAttempts << " attempts" << '\n';
			}
		}
	});
}

Client::~Client()
{
	Shutdown();
}

void Client::Shutdown()
{
	try
	{
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
	_subs.push_back(_events->AddListener(_name, [this](const NetworkEndFrameEvent&)
	{
		auto batch{std::make_shared<CommandBatch>()};
		{
			std::scoped_lock lock(_batchWriteMutex);
			std::swap(batch, _batch);
		}

		if (batch && !batch->IsEmpty())
		{
			SendCommand(batch);
		}
	}));

	// NOTE: local dispatch is keyed (MoveUpEvent/"P2") but the wire format sent via SendKeyState is
	// unchanged ("P2_Move_Up" etc.) - Session::OnKeyStateChange on the host still parses that
	// literal tag+action string out of the KeyStateChange command payload.
	_subs.push_back(_events->AddListener(std::string{"P2"}, _name, [this](const MoveUpEvent& event)
	{
		this->SendKeyState("P2_Move_Up", event.isPressed);
	}));
	_subs.push_back(_events->AddListener(std::string{"P2"}, _name, [this](const MoveLeftEvent& event)
	{
		this->SendKeyState("P2_Move_Left", event.isPressed);
	}));
	_subs.push_back(_events->AddListener(std::string{"P2"}, _name, [this](const MoveDownEvent& event)
	{
		this->SendKeyState("P2_Move_Down", event.isPressed);
	}));
	_subs.push_back(_events->AddListener(std::string{"P2"}, _name, [this](const MoveRightEvent& event)
	{
		this->SendKeyState("P2_Move_Right", event.isPressed);
	}));
	_subs.push_back(_events->AddListener(std::string{"P2"}, _name, [this](const FireEvent& event)
	{
		this->SendKeyState("P2_Fire", event.isPressed);
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ClientOutReadyToPlayEvent&)
	{
		std::scoped_lock lock(this->_batchWriteMutex);
		this->_batch->AddCommand(std::make_shared<SignalEvent>("ClientOut_ReadyToPlay"));
	}));

	_subs.push_back(_events->AddListener(_name, [this](const ClientOutPauseStatusEvent& event)
	{
		std::scoped_lock lock(this->_batchWriteMutex);
		this->_batch->AddCommand(std::make_shared<KeyStateChange>("Pause_Released", event.isPaused));
	}));
}

void Client::ReadResponse()
{
	auto self(shared_from_this());
	auto lambda = [this, self](const boost::system::error_code& ec, const std::size_t length)
	{
		//NOTE: self is captured only to keep this Client alive for the duration of the async
		//operation (shared_from_this() lifetime extension) - never dereferenced explicitly
		std::ignore = self;

		if (ec)
		{
			_readBuffer.consume(length);

			if (ec != boost::asio::error::eof && ec != boost::asio::error::operation_aborted)
			{
				std::cerr << ec.message() << '\n';
			}
		}
		else
		{
			const std::string archiveData(buffers_begin(_readBuffer.data()),
										  buffers_begin(_readBuffer.data()) + length);

			_readBuffer.consume(length);

			ProcessReceivedData(archiveData);

			// Since we want to keep listening, initiate reading again
			this->ReadResponse();
		}
	};

	boost::asio::async_read_until(_socket, _readBuffer, "\n\n", std::move(lambda));
}

void Client::SendKeyState(const std::string& key, const bool state)
{
	// NetworkLogger::LogClientOut(state);
	std::scoped_lock lock(_batchWriteMutex);
	_batch->AddCommand(std::make_shared<KeyStateChange>(key, state));
}

void Client::OnPositionChange(const std::shared_ptr<Command>& command)
{
	if (const auto* cmd = dynamic_cast<PositionChange*>(command.get()))
	{
		const auto pos = cmd->GetPos();
		const auto dir = cmd->GetDir();
		const auto uuid = cmd->GetUuid();

		_commandQueue.Enqueue([this, pos, dir, uuid]()
		{
			_events->EmitEvent(Key(uuid), ClientInPosEvent{.pos = pos, .dir = dir});
		});
	}
}

void Client::OnTankShot(const std::shared_ptr<Command>& command)
{
	if (const auto* cmd = dynamic_cast<TankShot*>(command.get()))
	{
		const auto who = cmd->GetWho();
		const auto dir = cmd->GetDir();
		const auto uuid = cmd->GetUuid();

		_commandQueue.Enqueue([this, who, dir, uuid]()
		{
			_events->EmitEvent(Key(who), ClientInShotEvent{.dir = dir, .bulletUuid = uuid});
		});
	}
}

void Client::OnHealthChange(const std::shared_ptr<Command>& command)
{
	if (const auto* cmd = dynamic_cast<HealthChange*>(command.get()))
	{
		const auto uuid = cmd->GetUuid();
		const auto health = cmd->GetHealth();

		_commandQueue.Enqueue([this, uuid, health]()
		{
			_events->EmitEvent(Key(uuid), ClientInHealthEvent{.health = health});
		});
	}
}

void Client::OnDispose(const std::shared_ptr<Command>& command)
{
	if (const auto* cmd = dynamic_cast<Dispose*>(command.get()))
	{
		const auto uuid = cmd->GetUuid();

		_commandQueue.Enqueue([this, uuid]()
		{
			_events->EmitEvent(Key(uuid), ClientInDisposeEvent{});
		});
	}
}

void Client::OnStatisticsChange(const std::shared_ptr<Command>& command)
{
	if (const auto* cmd = dynamic_cast<StatisticsChange*>(command.get()))
	{
		const auto type = cmd->GetType();
		const auto author = cmd->GetAuthor();
		const auto fraction = cmd->GetFraction();

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
}

void Client::OnKeyStateChange(const std::shared_ptr<Command>& command)
{
	if (const auto* cmd = dynamic_cast<KeyStateChange*>(command.get()))
	{
		const auto keyState = cmd->GetKeyState();
		const auto isEnable = cmd->GetIsEnable();

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
}

void Client::OnGameStateChange(const std::shared_ptr<Command>& command)
{
	if (const auto* cmd = dynamic_cast<GameStateChange*>(command.get()))
	{
		const auto gameState = cmd->GetGameState();

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
}

void Client::OnFortressChange(const std::shared_ptr<Command>& command)
{
	if (const auto* cmd = dynamic_cast<FortressChange*>(command.get()))
	{
		const std::string state = cmd->GetState();
		const auto uuid = cmd->GetUuid();

		_commandQueue.Enqueue([this, state, uuid]()
		{
			_events->EmitEvent(ClientInFortressChangeEvent{.state = state, .uuid = uuid});
		});
	}
}

void Client::OnBonusSpawn(const std::shared_ptr<Command>& command)
{
	if (const auto* cmd = dynamic_cast<BonusSpawn*>(command.get()))
	{
		const auto pos = cmd->GetPos();
		const auto bonusType = cmd->GetBonusType();
		const auto uuid = cmd->GetUuid();

		_commandQueue.Enqueue([this, pos, bonusType, uuid]()
		{
			_events->EmitEvent(ClientInBonusSpawnEvent{.pos = pos, .type = bonusType, .uuid = uuid});
		});
	}
}

void Client::OnBonusDeSpawn(const std::shared_ptr<Command>& command)
{
	if (const auto* cmd = dynamic_cast<BonusDeSpawn*>(command.get()))
	{
		const auto uuid = cmd->GetUuid();

		_commandQueue.Enqueue([this, uuid]()
		{
			_events->EmitEvent(ClientInBonusDeSpawnEvent{.uuid = uuid});
		});
	}
}

void Client::OnRespawnTank(const std::shared_ptr<Command>& command)
{
	if (const auto* cmd = dynamic_cast<RespawnTank*>(command.get()))
	{
		const auto tankType = cmd->GetTankType();
		const auto uuid = cmd->GetUuid();
		const auto rect = cmd->GetRect();

		_commandQueue.Enqueue([this, tankType, uuid, rect]()
		{
			_events->EmitEvent(ClientInRespawnTankEvent{.type = tankType, .uuid = uuid, .rect = rect});
		});
	}
}

void Client::OnObstacleSpawn(const std::shared_ptr<Command>& command)
{
	if (const auto* cmd = dynamic_cast<ObstacleSpawn*>(command.get()))
	{
		const auto rect = cmd->GetRect();
		const auto obstacleType = cmd->GetObstacleType();
		const auto uuid = cmd->GetUuid();

		_commandQueue.Enqueue([this, rect, obstacleType, uuid]()
		{
			_events->EmitEvent(ClientInObstacleSpawnEvent{.rect = rect, .type = obstacleType, .uuid = uuid});
		});
	}
}

void Client::OnAnimationCreate(const std::shared_ptr<Command>& command)
{
	if (const auto* cmd = dynamic_cast<AnimationCreate*>(command.get()))
	{
		const auto animationType = cmd->GetAnimationType();
		const auto rect = cmd->GetRect();
		const auto name = cmd->GetName();

		_commandQueue.Enqueue([this, animationType, rect, name]()
		{
			_events->EmitEvent(AnimationCreateEvent{.type = animationType, .rect = rect, .name = name});
		});
	}
}

void Client::OnTankOnOff(const std::shared_ptr<Command>& command)
{
	if (const auto* cmd = dynamic_cast<TankOnOff*>(command.get()))
	{
		const auto uuid = cmd->GetUuid();
		const auto isEnable = cmd->GetIsEnable();

		_commandQueue.Enqueue([this, uuid, isEnable]()
		{
			_events->EmitEvent(Key(uuid), ClientInOnTankOnOffEvent{.isEnable = isEnable});
		});
	}
}

void Client::OnCommandBatch(const std::shared_ptr<Command>& commands)
{
	if (const auto* cmd = dynamic_cast<CommandBatch*>(commands.get()))
	{
		for (const auto& command: cmd->GetCommands())
		{
			ProcessClientCommand(command);
		}
	}
}

void Client::OnBonusStatus(const std::shared_ptr<Command>& command)
{
	if (const auto* cmd = dynamic_cast<BonusStatus*>(command.get()))
	{
		const auto bonusType = cmd->GetBonusType();
		const auto name = cmd->GetName();
		const auto isEnable = cmd->GetIsEnable();

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
}

void Client::ProcessClientCommand(const std::shared_ptr<Command>& command)
{
	if (command)
	{
		// auto classNameW = std::string(command->GetClassNameW());
		// auto commandName = std::string("client receive:" + classNameW);
		// NetworkLogger::LogClientIn(commandName);
		switch (command->GetType())
		{
			case CommandType::COMMAND_BATCH:
			{
				OnCommandBatch(command);
				break;
			}
			case CommandType::POSITION_CHANGE:
			{
				OnPositionChange(command);
				//TODO: use more polymorphic way to process commands, uni method onReceived
				break;
			}
			case CommandType::TANK_SHOT:
			{
				OnTankShot(command);//TODO: refactored tankShot event to bullet pool spawn with bulletId
				break;
			}
			case CommandType::HEALTH_CHANGE:
			{
				OnHealthChange(command);
				break;
			}
			case CommandType::DISPOSE:
			{
				OnDispose(command);
				break;
			}
			case CommandType::STATISTICS_CHANGE:
			{
				OnStatisticsChange(command);
				break;
			}
			case CommandType::KEY_STATE_CHANGE:
			{
				OnKeyStateChange(command);
				break;
			}
			case CommandType::GAME_STATE_CHANGE:
			{
				OnGameStateChange(command);
				break;
			}
			case CommandType::FORTRESS_CHANGE:
			{
				OnFortressChange(command);
				break;
			}
			case CommandType::BONUS_SPAWN:
			{
				OnBonusSpawn(command);
				break;
			}
			case CommandType::BONUS_DESPAWN:
			{
				OnBonusDeSpawn(command);
				break;
			}
			case CommandType::RESPAWN_TANK:
			{
				OnRespawnTank(command);
				break;
			}
			case CommandType::OBSTACLE_SPAWN:
			{
				OnObstacleSpawn(command);
				break;
			}
			case CommandType::ANIMATION_CREATE:
			{
				OnAnimationCreate(command);
				break;
			}
			case CommandType::TANK_ON_OFF:
			{
				OnTankOnOff(command);
				break;
			}
			case CommandType::BONUS_STATUS:
			{
				OnBonusStatus(command);
				break;
			}
			//TODO: implement other command types
			default:
				break;
		}
	}
}

void Client::ProcessReceivedData(const std::string& archiveData)
{
	try
	{
		std::istringstream archiveStream(archiveData);
		boost::archive::text_iarchive ia(archiveStream);

		std::shared_ptr<Command> command;
		ia >> command;

		// NetworkLogger::WriteLog("\nraw data: " + archiveData+" =", true);
		ProcessClientCommand(command);
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

void Client::SendCommand(const std::shared_ptr<Command>& command)
{
	if (!_isConnected)
	{
		//NOTE: not an invariant violation - reconnect windows/disconnects are expected at runtime,
		//so log instead of asserting
		NetworkLogger::WriteLog("Client::SendCommand: dropped, not connected");
		return;
	}

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << command;

	{
		std::ostream os(&_writeBuffer);
		os << archiveStream.str() + "\n\n";
	}

	auto self(shared_from_this());
	auto lambda = [this, self](const boost::system::error_code& ec, const std::size_t length)
	{
		//NOTE: self is captured only to keep this Client alive for the duration of the async
		//operation (shared_from_this() lifetime extension) - never dereferenced explicitly
		std::ignore = self;

		_writeBuffer.consume(length);

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

			_isConnected = false;
			_socket.close();
		}
	};

	boost::asio::async_write(_socket, _writeBuffer.data(), std::move(lambda));
}
}//namespace network::commands
