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
	Client(boost::asio::io_context& ioContext, std::string host, uint16_t port,
		   const std::shared_ptr<EventSystem>& events);

	~Client();

	[[nodiscard]] network::NetworkCommandQueue& GetCommandQueue() { return _commandQueue; }

private:
	void Subscribe();
	void Unsubscribe() const;

	void ReadResponse();
	void TryConnect();

	void SendKeyState(const std::string& key, bool state);
	void OnPositionChange(const std::shared_ptr<Command>& command);
	void OnTankShot(const std::shared_ptr<Command>& command);
	void OnHealthChange(const std::shared_ptr<Command>& command);
	void OnDispose(const std::shared_ptr<Command>& command);
	void OnStatisticsChange(const std::shared_ptr<Command>& command);
	void OnKeyStateChange(const std::shared_ptr<Command>& command);
	void OnGameStateChange(const std::shared_ptr<Command>& command);
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

	tcp::socket _socket;
	boost::asio::steady_timer _reconnectTimer;
	tcp::endpoint _endpoint;
	boost::asio::streambuf _readBuffer{};
	boost::asio::streambuf _writeBuffer{};
	std::shared_ptr<EventSystem> _events{nullptr};
	std::string _name{};
	network::NetworkCommandQueue _commandQueue;
	bool _isConnected{false};
	int _reconnectAttempts{0};
	static constexpr int MaxReconnectAttempts{10};
	static constexpr int ReconnectDelayMs{500};
};
}//namespace network::commands
