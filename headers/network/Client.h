#pragma once

#include "enums/DisconnectReason.h"
#include "enums/InputSignal.h"
#include "PeerLink.h"
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
struct PauseRequestedEvent;

namespace network::commands
{
using boost::asio::ip::tcp;

class Client final : public PeerLink, public std::enable_shared_from_this<Client>
{
public:
	Client(boost::asio::io_context& ioContext, std::string host, uint16_t port,
		   const std::shared_ptr<EventSystem>& events);

	~Client();

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
	void OnPauseRequested(const PauseRequestedEvent& event);

	void SendKeyState(InputSignal action, bool state);
	void OnPositionChange(const AnyCommand& command);
	void OnTankShot(const AnyCommand& command);
	void OnHealthChange(const AnyCommand& command);
	void OnDespawn(const AnyCommand& command);
	void OnStatisticsChange(const AnyCommand& command);
	void OnKeyStateChange(const AnyCommand& command);
	void OnGameStateChange(const AnyCommand& command);
	void OnBonusSpawn(const AnyCommand& command);
	void OnRespawnTank(const AnyCommand& command);
	void OnObstacleSpawn(const AnyCommand& command);
	void OnTankSpawnComplete(const AnyCommand& command);
	void OnBonusStatus(const AnyCommand& command);
	void OnDisconnect(const AnyCommand& command);
	void HandleDisconnect();
	void HandleProtocolError();
	void ScheduleReconnect();

	boost::asio::steady_timer _reconnectTimer;
	tcp::endpoint _endpoint;
	std::vector<EventSubscription> _subs{};
	std::mutex _batchWriteMutex;
	CommandBatch _batch{};
	std::atomic<bool> _isConnected{};
	bool _reconnectPending{false};
	//NOTE: tells our own cancellation apart from a dropped link, so teardown does not reconnect
	std::atomic<bool> _isShuttingDown{false};
	//NOTE: same, from the other end - the EOF after a goodbye is expected, so no reconnect
	std::atomic<bool> _isHostGone{false};
	unsigned char _reconnectAttempts{0u};
	//NOTE: one drop can be reported twice, by the read and by the write - give up once
	bool _reconnectAbandoned{false};
	static constexpr unsigned char kMaxReconnectAttempts{10u};
	static constexpr unsigned short kReconnectDelayMs{500u};
};
}//namespace network::commands
