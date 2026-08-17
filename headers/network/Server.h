#pragma once

#include "CommandDispatcher.h"
#include "enums/DisconnectReason.h"
#include "enums/InputSignal.h"
#include "FrameChannel.h"
#include "NetworkCommandQueue.h"
#include "commands/CommandBatch.h"
#include "components/EventSystem.h"
#include <atomic>
#include <functional>
#include <unordered_map>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

class EventSystem;
struct NetworkEndFrameEvent;
struct ServerOutPauseStatusEvent;
struct ServerOutPlayersTeamIsWonEvent;
struct ServerOutEnemiesTeamIsWonEvent;
struct ServerOutPosEvent;
struct ServerOutShotEvent;
struct ServerOutHealthEvent;
struct ServerOutDisposeEvent;
struct ServerOutRespawnTankEvent;
struct ServerOutObstacleSpawnEvent;
struct ServerOutTankSpawnCompleteEvent;
struct ServerOutBulletHitEvent;
struct ServerOutEnemyHitEvent;
struct ServerOutPlayerOneHitEvent;
struct ServerOutPlayerTwoHitEvent;
struct ServerOutEnemyDiedEvent;
struct ServerOutPlayerOneDiedEvent;
struct ServerOutPlayerTwoDiedEvent;
struct ServerOutBrickWallDiedEvent;
struct ServerOutSteelWallDiedEvent;
struct ServerOutBonusPickupEvent;
struct ServerOutBonusDestroyedEvent;
struct ServerOutBonusSpawnEvent;
struct ServerOutBonusDeSpawnEvent;
struct ServerOutFortressChangeEvent;
struct ServerOutBonusHelmetPickupEvent;
struct ServerOutBonusStarPickupEvent;
struct ServerOutBonusCaliberPickupEvent;
struct ServerOutBonusTankPickupEvent;

namespace network::commands
{
using boost::asio::ip::tcp;

class Session final : public std::enable_shared_from_this<Session>
{
public:
	Session(tcp::socket sock, const std::shared_ptr<EventSystem>& events);

	~Session();

	[[nodiscard]] bool IsSocketOpen() const { return _channel->IsOpen(); }
	[[nodiscard]] bool HasPendingCommands() const { return _commandQueue.Size() > 0u; }
	void ProcessCommandQueue() { _commandQueue.ProcessAll(); }

	void Start();
	//NOTE: shared, not copied - the same frame goes to every session and stays alive while it is written
	void DoWrite(std::shared_ptr<const std::string> message);
	void Shutdown();

	//NOTE: onClosed fires once the goodbye is written, or turned out undeliverable
	void Shutdown(DisconnectReason reason, std::function<void()> onClosed);

private:
	using InputEmitter = std::function<void(EventSystem&, const std::string&, bool)>;

	void RegisterCommandHandlers();
	void OnSignalEvent(const AnyCommand& command);
	void OnKeyStateChange(const AnyCommand& command);
	void OnDisconnect(const AnyCommand& command);

	static const std::unordered_map<InputSignal, InputEmitter> kInputEmitters;

	std::shared_ptr<network::FrameChannel> _channel;
	std::shared_ptr<EventSystem> _events{nullptr};
	network::NetworkCommandQueue _commandQueue;
	network::CommandDispatcher _dispatcher;
};

class Server final
{
public:
	Server(boost::asio::io_context& ioContext, std::string host, uint16_t port,
		   const std::shared_ptr<EventSystem>& events);

	~Server();

	void Shutdown();

	//NOTE: closes once every session is flushed
	void Shutdown(DisconnectReason reason, const std::function<void()>& onClosed);

	[[nodiscard]] uint16_t GetBoundPort() const { return _acceptor.local_endpoint().port(); }

	void ProcessNetworkCommands() const;

private:
	void DoAccept();

	[[nodiscard]] std::vector<std::shared_ptr<Session>> SnapshotSessions() const;

	void StartSendThread();
	void StopSendThread();

	void SendCommand(const CommandBatch& command);

	void Subscribe();
	void SubscribeStatistics();
	void SubscribeBonus();

	void OnNetworkEndFrame(const NetworkEndFrameEvent&);
	void OnPauseStatus(const ServerOutPauseStatusEvent& event);
	void OnPlayersTeamIsWon(const ServerOutPlayersTeamIsWonEvent&);
	void OnEnemiesTeamIsWon(const ServerOutEnemiesTeamIsWonEvent&);
	void OnPos(const ServerOutPosEvent& event);
	void OnShot(const ServerOutShotEvent& event);
	void OnHealth(const ServerOutHealthEvent& event);
	void OnDispose(const ServerOutDisposeEvent& event);
	void OnRespawnTank(const ServerOutRespawnTankEvent& event);
	void OnObstacleSpawn(const ServerOutObstacleSpawnEvent& event);
	void OnTankSpawnComplete(const ServerOutTankSpawnCompleteEvent& event);

	void OnBulletHit(const ServerOutBulletHitEvent& event);
	void OnEnemyHit(const ServerOutEnemyHitEvent& event);
	void OnPlayerOneHit(const ServerOutPlayerOneHitEvent& event);
	void OnPlayerTwoHit(const ServerOutPlayerTwoHitEvent& event);
	void OnEnemyDied(const ServerOutEnemyDiedEvent& event);
	void OnPlayerOneDied(const ServerOutPlayerOneDiedEvent& event);
	void OnPlayerTwoDied(const ServerOutPlayerTwoDiedEvent& event);
	void OnBrickWallDied(const ServerOutBrickWallDiedEvent& event);
	void OnSteelWallDied(const ServerOutSteelWallDiedEvent& event);
	void OnBonusPickup(const ServerOutBonusPickupEvent& event);
	void OnBonusDestroyed(const ServerOutBonusDestroyedEvent& event);

	void OnBonusSpawn(const ServerOutBonusSpawnEvent& event);
	void OnBonusDeSpawn(const ServerOutBonusDeSpawnEvent& event);
	void OnFortressChange(const ServerOutFortressChangeEvent& event);
	void OnBonusHelmetPickup(const ServerOutBonusHelmetPickupEvent& event);
	void OnBonusStarPickup(const ServerOutBonusStarPickupEvent& event);
	void OnBonusCaliberPickup(const ServerOutBonusCaliberPickupEvent& event);
	void OnBonusTankPickup(const ServerOutBonusTankPickupEvent& event);

	void SendToAll(const std::shared_ptr<const std::string>& message);
	void CleanupDeadSessions();
	void CloseAcceptor();

	tcp::acceptor _acceptor;
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	//NOTE: reached by the io_context, send and main threads; readers copy it via SnapshotSessions
	//and work outside the lock, so no foreign code ever runs while it is held
	std::vector<std::shared_ptr<Session>> _sessions;
	mutable std::mutex _sessionsMutex;

	std::mutex _batchWriteMutex;
	CommandBatch _batch{};

	std::queue<CommandBatch> _sendQueue;
	std::mutex _sendQueueMutex;
	std::condition_variable _sendCondition;
	std::thread _sendThread;
	std::atomic<bool> _isRunning{false};
};
}//namespace network::commands
