#pragma once

#include "NetworkCommandQueue.h"
#include "commands/Command.h"
#include <boost/asio.hpp>
#include <memory>
#include <string>

class EventSystem;
enum class BonusType : char8_t;
enum class Direction : char8_t;

namespace network::commands
{
class BaseObj;
using boost::asio::ip::tcp;

class Client final : public std::enable_shared_from_this<Client>
{
public:
	Client(boost::asio::io_context& ioContext, const std::string& host, const std::string& port,
		   const std::shared_ptr<EventSystem>& events);

	~Client();

	void Subscribe();
	void Unsubscribe() const;

	void ReadResponse();

	void SendKeyState(const std::string& state);
	void OnPositionChange(const std::shared_ptr<Command>& command);
	void OnTankShot(const std::shared_ptr<Command>& command);
	void OnHealthChange(const std::shared_ptr<Command>& command);
	void OnDispose(const std::shared_ptr<Command>& command);
	void OnStatisticsChange(const std::shared_ptr<Command>& command);
	void OnKeyStateChange(const std::shared_ptr<Command>& command);
	void OnFortressChange(const std::shared_ptr<Command>& command);
	void OnBonusSpawn(const std::shared_ptr<Command>& command);
	void OnBonusDeSpawn(const std::shared_ptr<Command>& command);
	void OnRespawnTank(const std::shared_ptr<Command>& command);
	void OnObstacleSpawn(const std::shared_ptr<Command>& command);
	void OnAnimationCreate(const std::shared_ptr<Command>& command);
	void OnTankOnOff(const std::shared_ptr<Command>& command);
	void OnCommandBatch(const std::shared_ptr<Command>& commands);
	void OnBonusStatus(const std::shared_ptr<Command>& command);
	void ProcessClientCommand(const std::shared_ptr<Command>& command);
	void ProcessReceivedData(const std::string& archiveData);
	void SendCommand(const std::shared_ptr<Command>& command);

	[[nodiscard]] network::NetworkCommandQueue& GetCommandQueue() { return _commandQueue; }

private:
	tcp::socket _socket;
	boost::asio::streambuf _readBuffer{};
	boost::asio::streambuf _writeBuffer{};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::string _name{};
	network::NetworkCommandQueue _commandQueue;
};
}//namespace network::commands
