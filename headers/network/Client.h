#pragma once

#include "NetworkCommandQueue.h"
#include "commands/Command.h"
#include "commands/CommandBatch.h"
#include "components/EventSystem.h"
#include <atomic>
#include <boost/asio.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>

class EventSystem;
enum class BonusType : char8_t;
enum class Direction : char8_t;
struct NetworkEndFrameEvent;
struct MoveUpEvent;
struct MoveLeftEvent;
struct MoveDownEvent;
struct MoveRightEvent;
struct FireEvent;
struct ClientOutReadyToPlayEvent;
struct ClientOutPauseStatusEvent;

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
	[[nodiscard]] bool IsConnected() const { return _isConnected; }

	void Shutdown();

private:
	using CommandHandler = std::function<void(const std::shared_ptr<Command>&)>;

	void Subscribe();
	void RegisterCommandHandlers();

	void ReadResponse();
	void TryConnect();

	void OnNetworkEndFrame(const NetworkEndFrameEvent&);
	void OnMoveUp(const MoveUpEvent& event);
	void OnMoveLeft(const MoveLeftEvent& event);
	void OnMoveDown(const MoveDownEvent& event);
	void OnMoveRight(const MoveRightEvent& event);
	void OnFire(const FireEvent& event);
	void OnClientOutReadyToPlay(const ClientOutReadyToPlayEvent&);
	void OnClientOutPauseStatus(const ClientOutPauseStatusEvent& event);

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
	void OnTankSpawnComplete(const std::shared_ptr<Command>& command);
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
	std::vector<EventSubscription> _subs{};
	network::NetworkCommandQueue _commandQueue;
	std::mutex _batchWriteMutex;
	std::shared_ptr<CommandBatch> _batch{nullptr};
	std::unordered_map<CommandType, CommandHandler> _commandHandlers{};
	std::atomic<bool> _isConnected{false};
	unsigned char _reconnectAttempts{0u};
	static constexpr unsigned char MaxReconnectAttempts{10u};
	static constexpr unsigned short ReconnectDelayMs{500u};
};
}//namespace network::commands
