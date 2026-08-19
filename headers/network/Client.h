#pragma once

#include "CommandDispatcher.h"
#include "FrameChannel.h"
#include "enums/DisconnectReason.h"
#include "enums/InputSignal.h"
#include "NetworkCommandQueue.h"
#include "commands/CommandBatch.h"
#include "components/EventSystem.h"
#include <atomic>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/asio/steady_timer.hpp>
#include <boost/asio/strand.hpp>
#include <functional>
#include <memory>
#include <mutex>
#include <string>
#include <vector>

class EventSystem;
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
using boost::asio::ip::tcp;

class Client final : public std::enable_shared_from_this<Client>
{
public:
	Client(boost::asio::io_context& ioContext, std::string host, uint16_t port,
		   const std::shared_ptr<EventSystem>& events);

	~Client();

	void ProcessCommandQueue() { _commandQueue.ProcessAll(); }
	[[nodiscard]] bool IsConnected() const { return _isConnected; }

	void Shutdown();

	//NOTE: onClosed fires once the goodbye is on the wire, or turned out undeliverable
	void Shutdown(DisconnectReason reason, std::function<void()> onClosed);

private:
	void Subscribe();
	void RegisterCommandHandlers();

	void StartReading();
	void TryConnect();

	void OnNetworkEndFrame(const NetworkEndFrameEvent&);
	void OnMoveUp(const MoveUpEvent& event);
	void OnMoveLeft(const MoveLeftEvent& event);
	void OnMoveDown(const MoveDownEvent& event);
	void OnMoveRight(const MoveRightEvent& event);
	void OnFire(const FireEvent& event);
	void OnClientOutReadyToPlay(const ClientOutReadyToPlayEvent&);
	void OnClientOutPauseStatus(const ClientOutPauseStatusEvent& event);

	void SendKeyState(InputSignal action, bool state);
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
	void OnDisconnect(const AnyCommand& command);
	void SendCommand(const CommandBatch& command);
	//NOTE: idempotent - a read error and a write error can both report the same drop
	void HandleDisconnect();
	//NOTE: unlike HandleDisconnect, deliberately does not reconnect - see the definition
	void HandleProtocolError();
	void ScheduleReconnect();

	//NOTE: channel socket and timer share it, so their handlers are serialised
	boost::asio::strand<boost::asio::io_context::executor_type> _strand;
	std::shared_ptr<network::FrameChannel> _channel;
	boost::asio::steady_timer _reconnectTimer;
	tcp::endpoint _endpoint;
	std::shared_ptr<EventSystem> _events{};
	std::vector<EventSubscription> _subs{};
	network::NetworkCommandQueue _commandQueue;
	std::mutex _batchWriteMutex;
	CommandBatch _batch{};
	network::CommandDispatcher _dispatcher;
	std::atomic<bool> _isConnected{};
	bool _reconnectPending{false};
	//NOTE: tells our own cancellation apart from a dropped link, so teardown does not reconnect
	std::atomic<bool> _isShuttingDown{false};
	//NOTE: same, from the other end - the EOF after a goodbye is expected, so no reconnect
	std::atomic<bool> _isHostGone{false};
	unsigned char _reconnectAttempts{0u};
	static constexpr unsigned char MaxReconnectAttempts{10u};
	static constexpr unsigned short ReconnectDelayMs{500u};
};
}//namespace network::commands
