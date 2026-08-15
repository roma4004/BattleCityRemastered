#pragma once

#include "NetworkCommandQueue.h"
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
	using CommandHandler = std::function<void(const AnyCommand&)>;

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
	void OnPositionChange(const AnyCommand& command);
	void OnTankShot(const AnyCommand& command);
	void OnHealthChange(const AnyCommand& command);
	void OnDispose(const AnyCommand& command);
	void OnStatisticsChange(const AnyCommand& command);
	void OnKeyStateChange(const AnyCommand& command);
	void OnGameStateChange(const AnyCommand& command);
	void OnFortressChange(const AnyCommand& command);
	void OnBonusSpawn(const AnyCommand& command);
	void OnBonusDeSpawn(const AnyCommand& command);
	void OnRespawnTank(const AnyCommand& command);
	void OnObstacleSpawn(const AnyCommand& command);
	void OnTankSpawnComplete(const AnyCommand& command);
	void OnBonusStatus(const AnyCommand& command);
	void ProcessClientCommand(const AnyCommand& command);
	void ProcessReceivedData(const std::string& archiveData);
	void SendCommand(const CommandBatch& command);

	tcp::socket _socket;
	boost::asio::steady_timer _reconnectTimer;
	tcp::endpoint _endpoint;
	boost::asio::streambuf _readBuffer{};
	boost::asio::streambuf _writeBuffer{};
	std::shared_ptr<EventSystem> _events{};
	std::vector<EventSubscription> _subs{};
	network::NetworkCommandQueue _commandQueue;
	std::mutex _batchWriteMutex;
	CommandBatch _batch{};
	std::unordered_map<CommandType, CommandHandler> _commandHandlers{};
	std::atomic<bool> _isConnected{};
	unsigned char _reconnectAttempts{0u};
	static constexpr unsigned char MaxReconnectAttempts{10u};
	static constexpr unsigned short ReconnectDelayMs{500u};
};
}//namespace network::commands
