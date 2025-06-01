#include "../../headers/network/Client.h"

#include "../../headers/ObjRectangle.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/ComandType.h"
#include "../../headers/enums/TankType.h"
#include "../../headers/network/commands/BonusDeSpawn.h"
#include "../../headers/network/commands/BonusSpawn.h"
#include "../../headers/network/commands/Command.h"
#include "../../headers/network/commands/CommandBatch.h"
#include "../../headers/network/commands/Dispose.h"
#include "../../headers/network/commands/FortressChange.h"
#include "../../headers/network/commands/HealthChange.h"
#include "../../headers/network/commands/KeyStateChange.h"
#include "../../headers/network/commands/ObstacleSpawn.h"
#include "../../headers/network/commands/PositionChange.h"
#include "../../headers/network/commands/RespawnTank.h"
#include "../../headers/network/commands/StatisticsChange.h"
#include "../../headers/network/commands/TankShot.h"

#include <fstream>
#include <iostream>
#include <string>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/uuid/uuid_io.hpp>

// std::ofstream error_log("error_log_client.txt");

Client::Client(boost::asio::io_context& ioContext, const std::string& host, const std::string& port,
               std::shared_ptr<EventSystem> events)
	: _socket(ioContext),
	  _events{std::move(events)},
	  _name{"Client"}
{
	Subscribe();

	tcp::resolver resolver(ioContext);
	const auto endpointIterator = resolver.resolve(host, port);
	boost::asio::async_connect(
			_socket, endpointIterator,
			[this](const boost::system::error_code& ec, tcp::endpoint /*endpoint_iterator*/)
			{
				if (!ec)
				{
					// Init connection success, start reading from socket
					//_events->EmitEvent("ClientConnectionEstablished");
					this->ReadResponse();
				}
			});
}

Client::~Client()
{
	Unsubscribe();

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
				std::cerr << "Error during socket close: " << ec.message() << std::endl;
			}
		}
	}
	catch (const std::exception& e)
	{
		std::cerr << "Exception in ~Client: " << e.what() << '\n';
	}
	catch (...)
	{
		std::cerr << "Unknown error in ~Client" << '\n';
	}
}

void Client::Subscribe()
{
	_events->AddListener("ArrowUp_Pressed", _name, [this]() { this->SendKeyState("ArrowUp_Pressed"); });
	_events->AddListener("ArrowLeft_Pressed", _name, [this]() { this->SendKeyState("ArrowLeft_Pressed"); });
	_events->AddListener("ArrowDown_Pressed", _name, [this]() { this->SendKeyState("ArrowDown_Pressed"); });
	_events->AddListener("ArrowRight_Pressed", _name, [this]() { this->SendKeyState("ArrowRight_Pressed"); });
	_events->AddListener("RCTRL_Pressed", _name, [this]() { this->SendKeyState("RCTRL_Pressed"); });

	_events->AddListener("ArrowUp_Released", _name, [this]() { this->SendKeyState("ArrowUp_Released"); });
	_events->AddListener("ArrowLeft_Released", _name, [this]() { this->SendKeyState("ArrowLeft_Released"); });
	_events->AddListener("ArrowDown_Released", _name, [this]() { this->SendKeyState("ArrowDown_Released"); });
	_events->AddListener("ArrowRight_Released", _name, [this]() { this->SendKeyState("ArrowRight_Released"); });
	_events->AddListener("RCTRL_Released", _name, [this]() { this->SendKeyState("RCTRL_Released"); });

	_events->AddListener("ClientReadyToPlay", _name, [this]() { this->SendKeyState("ClientReadyToPlay"); });
}

void Client::Unsubscribe() const
{
	_events->RemoveListener("ArrowUp_Pressed", _name);
	_events->RemoveListener("ArrowLeft_Pressed", _name);
	_events->RemoveListener("ArrowDown_Pressed", _name);
	_events->RemoveListener("ArrowRight_Pressed", _name);
	_events->RemoveListener("RCTRL_Pressed", _name);

	_events->RemoveListener("ArrowUp_Released", _name);
	_events->RemoveListener("ArrowLeft_Released", _name);
	_events->RemoveListener("ArrowDown_Released", _name);
	_events->RemoveListener("ArrowRight_Released", _name);
	_events->RemoveListener("RCTRL_Released", _name);
}

void Client::ReadResponse()
{
	// auto self(shared_from_this());
	auto lambda = [this, events = _events](const boost::system::error_code& ec, const std::size_t length)
	{
		if (ec)
		{
			_read_buffer.consume(length);
			std::cerr << ec.message() << '\n';
		}
		else
		{
			const std::string archiveData(buffers_begin(_read_buffer.data()),
			                              buffers_begin(_read_buffer.data()) + length);

			_read_buffer.consume(length);

			ProcessReceivedData(archiveData);

			// static bool isFirstRead = true;
			// if (isFirstRead) {
			// 	// events->EmitEvent("ClientConnected");
			// 	isFirstRead = false;
			// }

			// if (data.eventName == "OnHelmetActivate")
			// {
			// 	events->EmitEvent("ClientReceived_" + data.who + data.eventName);
			// }
			// else if (data.eventName == "OnHelmetDeactivate")
			// {
			// 	events->EmitEvent("ClientReceived_" + data.who + data.eventName);
			// }
			// else if (data.eventName == "OnStar")
			// {
			// 	events->EmitEvent("ClientReceived_" + data.who + data.eventName);
			// }
			// else if (data.eventName == "OnTank")
			// {
			// 	events->EmitEvent<const std::string&, const std::string&>(
			// 			"ClientReceived_" + data.eventName, data.who, data.fraction);
			// }
			// else if (data.eventName == "OnGrenade")
			// {
			// 	events->EmitEvent<const std::string&, const std::string&>(
			// 			"ClientReceived_" + data.eventName, data.who, data.fraction);
			// }

			// Since we want to keep listening, initiate reading again
			this->ReadResponse();
		}
	};

	boost::asio::async_read_until(_socket, _read_buffer, "\n\n", std::move(lambda));
}

void Client::SendKeyState(const std::string& state)
{
	// auto self(shared_from_this());
	ClientData data;
	data.health = 1;
	data.eventName = state;
	data.names = {"Name1", "Name2"};

	std::ostringstream archiveStream;
	boost::archive::text_oarchive oa(archiveStream);
	oa << data;

	{
		std::ostream os(&_write_buffer);
		os << archiveStream.str() + "\n\n";
	}

	boost::asio::async_write(
			_socket,
			_write_buffer,
			[this](const boost::system::error_code& ec, const std::size_t length)
			{
				_write_buffer.consume(length);// Now we can consume the written data
				if (!ec)
				{
					// Response send success!
					// this->ReadResponse();
				}
			});
}

void Client::OnPositionChange(const std::shared_ptr<Command>& command) const
{
	if (const auto* cmd = dynamic_cast<PositionChange*>(command.get()))
	{
		_events->EmitEvent<const FPoint, const Direction, const boost::uuids::uuid>(
				"ClientReceived_" + cmd->GetWho() + "Pos", cmd->GetPos(), cmd->GetDir(), cmd->GetUuid());
	}
}

void Client::OnTankShot(const std::shared_ptr<Command>& command) const
{
	if (const auto* cmd = dynamic_cast<TankShot*>(command.get()))
	{
		const boost::uuids::uuid uuid = cmd->GetUuid();
		_events->EmitEvent<const Direction, const boost::uuids::uuid>(
				"ClientReceived_" + cmd->GetWho() + "Shot", cmd->GetDir(), uuid);
	}
}

void Client::OnHealthChange(const std::shared_ptr<Command>& command) const
{
	if (const auto* cmd = dynamic_cast<HealthChange*>(command.get()))
	{
		_events->EmitEvent<const int>(
				"ClientReceived_" + cmd->GetWho() + boost::uuids::to_string(cmd->GetUuid()) + "Health",
				cmd->GetHealth());
	}
}

void Client::OnDispose(const std::shared_ptr<Command>& command) const
{
	if (const auto* cmd = dynamic_cast<Dispose*>(command.get()))
	{
		_events->EmitEvent<const boost::uuids::uuid&>("ClientReceived_" + cmd->GetWho() + "Dispose", cmd->GetUuid());
	}
}

void Client::OnStatisticsChange(const std::shared_ptr<Command>& command) const
{
	if (const auto* cmd = dynamic_cast<StatisticsChange*>(command.get()))
	{
		_events->EmitEvent<const std::string&, const std::string&, const std::string&>(
				"ClientReceived_Statistics", cmd->GetEventName(), cmd->GetAuthor(), cmd->GetFraction());
	}
}

// void Client::OnKeyStateChange<//TODO: template this>(const std::shared_ptr<Command>& command) const
void Client::OnKeyStateChange(const std::shared_ptr<Command>& command) const
{
	if (const auto* cmd = dynamic_cast<KeyStateChange*>(command.get()))
	{
		_events->EmitEvent(cmd->GetKeyState());
	}
}

void Client::OnFortressChange(const std::shared_ptr<Command>& command) const
{
	if (const auto* cmd = dynamic_cast<FortressChange*>(command.get()))
	{
		const boost::uuids::uuid uuid = cmd->GetUuid();
		_events->EmitEvent<const std::string&, const boost::uuids::uuid>(
				"ClientReceived_FortressChange", cmd->GetState(), uuid);
	}
}

void Client::OnBonusSpawn(const std::shared_ptr<Command>& command) const
{
	if (const auto* cmd = dynamic_cast<BonusSpawn*>(command.get()))
	{
		_events->EmitEvent<const FPoint, const BonusType, const boost::uuids::uuid>(
				"ClientReceived_BonusSpawn", cmd->GetPos(), cmd->GetBonusType(), cmd->GetUuid());
	}
}

void Client::OnBonusDeSpawn(const std::shared_ptr<Command>& command) const
{
	if (const auto* cmd = dynamic_cast<BonusDeSpawn*>(command.get()))
	{
		_events->EmitEvent<const boost::uuids::uuid&>("ClientReceived_BonusDeSpawn", cmd->GetUuid());
	}
}

void Client::OnRespawnTank(const std::shared_ptr<Command>& command) const
{
	if (const auto* cmd = dynamic_cast<RespawnTank*>(command.get()))
	{
		_events->EmitEvent<const TankType, const boost::uuids::uuid>(
				"ClientReceived_RespawnTank", cmd->GetTankType(), cmd->GetUuid());
	}
}

void Client::OnObstacleSpawn(const std::shared_ptr<Command>& command) const
{
	if (const auto* cmd = dynamic_cast<ObstacleSpawn*>(command.get()))
	{
		_events->EmitEvent<const ObjRectangle, const ObstacleType, const boost::uuids::uuid>(
				"ClientReceived_ObstacleSpawn", cmd->GetRect(), cmd->GetObstacleType(), cmd->GetUuid());
	}
}

void Client::OnCommandBatch(const std::shared_ptr<Command>& commands) const
{
	if (const auto* cmd = dynamic_cast<CommandBatch*>(commands.get()))
	{
		for (const auto& command: cmd->GetCommands())
		{
			ProcessClientCommand(command);
		}
	}
}

void Client::ProcessClientCommand(const std::shared_ptr<Command>& command) const
{
	if (command)
	{
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
			//TODO: implement other command types
			default:
				break;
		}
	}
}

void Client::ProcessReceivedData(const std::string& archiveData) const
{
	try
	{
		std::istringstream archiveStream(archiveData);
		boost::archive::text_iarchive ia(archiveStream);

		std::shared_ptr<Command> command;
		ia >> command;

		ProcessClientCommand(command);
	}
	catch (const std::exception& e)
	{
		std::cerr << "Deserialization error: " << e.what() << std::endl;
		std::cerr << "Raw data: " << archiveData << std::endl;
	}
}
