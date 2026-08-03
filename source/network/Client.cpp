#include "network/Client.h"
#include "components/EventSystem.h"
#include "components/SpawnEvents.h"
#include "entities/ObjRectangle.h"
#include "enums/CommandType.h"
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
#include "utils/UuidUtils.h"
// #include <fstream>
#include "enums/AnimationType.h"
#include "network/commands/AnimationCreate.h"
#include "network/commands/BonusStatus.h"
#include "network/commands/GameStateChange.h"
#include "network/commands/SignalEvent.h"
#include "network/commands/TankOnOff.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <iostream>
#include <string>

namespace network::commands
{
Client::Client(boost::asio::io_context& ioContext, std::string host, uint16_t port,
			   const std::shared_ptr<EventSystem>& events)
	: _socket(ioContext)
	, _reconnectTimer(ioContext)
	, _endpoint{tcp::endpoint(boost::asio::ip::make_address(host), port)}
	, _events{events}
	, _name{"Client"}
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
		const auto result = _socket.close(ec);
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
			// std::scoped_lock lock(_batchWriteMutex);
			// this->_batch->AddCommand(  //TODO: implement batch sending
			this->SendCommand(std::make_shared<SignalEvent>("ClientSend_ReadyToPlay"));
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
	Unsubscribe();
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
	//TODO: write batch sending on client and sending queue
	_events->AddListener("P2_Move_Up", _name, [this](const bool isPressed)
	{
		this->SendKeyState("P2_Move_Up", isPressed);
	});
	_events->AddListener("P2_Move_Left", _name, [this](const bool isPressed)
	{
		this->SendKeyState("P2_Move_Left", isPressed);
	});
	_events->AddListener("P2_Move_Down", _name, [this](const bool isPressed)
	{
		this->SendKeyState("P2_Move_Down", isPressed);
	});
	_events->AddListener("P2_Move_Right", _name, [this](const bool isPressed)
	{
		this->SendKeyState("P2_Move_Right", isPressed);
	});
	_events->AddListener("P2_Fire", _name, [this](const bool isPressed) { this->SendKeyState("P2_Fire", isPressed); });

	_events->AddListener("ClientSend_ReadyToPlay", _name, [this]()
	{
		// std::scoped_lock lock(_batchWriteMutex);
		// this->_batch->AddCommand(  //TODO: implement batch sending
		SendCommand(std::make_shared<SignalEvent>("ClientSend_ReadyToPlay"));
	});

	_events->AddListener("ClientSend_Pause_Status", _name, [this](const bool isPaused)
	{
		// std::scoped_lock lock(_batchWriteMutex);
		// this->_batch->AddCommand(  //TODO: implement batch sending
		SendCommand(std::make_shared<KeyStateChange>("Pause_Released", isPaused));
	});
}

void Client::Unsubscribe() const { _events->RemoveAllListeners(_name); }

void Client::ReadResponse()
{
	auto self(shared_from_this());
	auto lambda = [this, self](const boost::system::error_code& ec, const std::size_t length)
	{
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
	// NetworkLogger::LogClientSend(state);
	// std::scoped_lock lock(_batchWriteMutex);
	// this->_batch->AddCommand(  //TODO: implement batch sending
	SendCommand(std::make_shared<KeyStateChange>(key, state));
}

void Client::OnPositionChange(const std::shared_ptr<Command>& command)
{
	if (const auto* cmd = dynamic_cast<PositionChange*>(command.get()))
	{
		const auto who = cmd->GetWho();
		const auto pos = cmd->GetPos();
		const auto dir = cmd->GetDir();
		const auto uuid = cmd->GetUuid();

		_commandQueue.Enqueue([this, who, pos, dir, uuid]()
		{
			_events->EmitEvent("ClientReceived_" + who + "Pos", pos, dir, uuid);
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
			_events->EmitEvent("ClientReceived_" + who + "Shot", dir, uuid);
		});
	}
}

void Client::OnHealthChange(const std::shared_ptr<Command>& command)
{
	if (const auto* cmd = dynamic_cast<HealthChange*>(command.get()))
	{
		const auto who = cmd->GetWho();
		const auto uuid = cmd->GetUuid();
		const auto health = cmd->GetHealth();

		_commandQueue.Enqueue([this, who, uuid, health]()
		{
			_events->EmitEvent("ClientReceived_" + who + UuidUtils::GetStringUuid(uuid) + "Health", health);
		});
	}
}

void Client::OnDispose(const std::shared_ptr<Command>& command)
{
	if (const auto* cmd = dynamic_cast<Dispose*>(command.get()))
	{
		const auto who = cmd->GetWho();
		const auto uuid = cmd->GetUuid();

		_commandQueue.Enqueue([this, who, uuid]()
		{
			_events->EmitEvent("ClientReceived_" + who + "Dispose", uuid);
		});
	}
}

void Client::OnStatisticsChange(const std::shared_ptr<Command>& command)
{
	if (const auto* cmd = dynamic_cast<StatisticsChange*>(command.get()))
	{
		const auto eventName = cmd->GetEventName();
		const auto author = cmd->GetAuthor();
		const auto fraction = cmd->GetFraction();

		_commandQueue.Enqueue([this, eventName, author, fraction]()
		{
			_events->EmitEvent("ClientReceived_Statistics", eventName, author, fraction);
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
			this->_events->EmitEvent(keyState, isEnable);
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
			this->_events->EmitEvent(gameState);
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
			_events->EmitEvent("ClientReceived_FortressChange", state, uuid);
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
			_events->EmitEvent("ClientReceived_BonusSpawn", ClientReceivedBonusSpawnEvent{pos, bonusType, uuid});
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
			_events->EmitEvent("ClientReceived_BonusDeSpawn", uuid);
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
			_events->EmitEvent("ClientReceived_RespawnTank", ClientReceivedRespawnTankEvent{tankType, uuid, rect});
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
			_events->EmitEvent("ClientReceived_ObstacleSpawn",
							   ClientReceivedObstacleSpawnEvent{rect, obstacleType, uuid});
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
			_events->EmitEvent("AnimationCreate", AnimationCreateEvent{animationType, rect, name});
		});
	}
}

void Client::OnTankOnOff(const std::shared_ptr<Command>& command)
{
	if (const auto* cmd = dynamic_cast<TankOnOff*>(command.get()))
	{
		const auto name = cmd->GetName();
		const auto uuid = cmd->GetUuid();
		const auto isEnable = cmd->GetIsEnable();

		_commandQueue.Enqueue([this, name, uuid, isEnable]()
		{
			_events->EmitEvent("ClientReceived_" + name + "OnTankOnOff", uuid, isEnable);
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
					_events->EmitEvent("ClientReceived_" + name + "BonusHelmet_Pickup", isEnable);
					break;
				case BonusType::Star:
					_events->EmitEvent("ClientReceived_" + name + "BonusStar_Pickup");
					break;
				case BonusType::Caliber:
					_events->EmitEvent("ClientReceived_" + name + "BonusCaliber_Pickup");
					break;
				case BonusType::Tank:
					_events->EmitEvent("ClientReceived_BonusTank_Pickup", name);
					break;
				default: //TODO: add assert
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
		// NetworkLogger::LogClientReceive(commandName);
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
		return;
		//TODO: add assert or console error print
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
