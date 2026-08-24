#pragma once

#include "enums/DisconnectReason.h"
#include "enums/InputSignal.h"
#include "PeerLink.h"
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
struct PauseStatusEvent;
struct ServerOutPlayersTeamIsWonEvent;
struct ServerOutEnemiesTeamIsWonEvent;
struct PosChangedEvent;
struct TankShotEvent;
struct HealthChangedEvent;
struct DespawnedEvent;
struct TankRespawnedEvent;
struct ObstacleSpawnedEvent;
struct TankSpawnCompletedEvent;
struct StatisticsBulletHitEvent;
struct StatisticsTankHitEvent;
struct StatisticsTankDiedEvent;
struct BrickWallDiedEvent;
struct SteelWallDiedEvent;
struct StatisticsBonusPickupEvent;
struct StatisticsBonusDestroyedEvent;
struct StatisticsBonusExpiredEvent;
struct BonusSpawnedEvent;
struct BonusHelmetAppliedEvent;
struct BonusStarAppliedEvent;
struct BonusCaliberAppliedEvent;
struct BonusTankAppliedEvent;

namespace network::commands
{
using boost::asio::ip::tcp;

class Session final : public PeerLink, public std::enable_shared_from_this<Session>
{
public:
	Session(tcp::socket sock, const std::shared_ptr<EventSystem>& events);

	~Session();

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

	bool _isPeerGone{false};
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
	void OnPauseStatus(const PauseStatusEvent& event);
	void OnPlayersTeamIsWon(const ServerOutPlayersTeamIsWonEvent&);
	void OnEnemiesTeamIsWon(const ServerOutEnemiesTeamIsWonEvent&);
	void OnPos(const PosChangedEvent& event);
	void OnShot(const TankShotEvent& event);
	void OnHealth(const HealthChangedEvent& event);
	void OnDespawn(const DespawnedEvent& event);
	void OnRespawnTank(const TankRespawnedEvent& event);
	void OnObstacleSpawn(const ObstacleSpawnedEvent& event);
	void OnTankSpawnComplete(const TankSpawnCompletedEvent& event);

	void OnBulletHit(const StatisticsBulletHitEvent& event);
	void OnTankHit(const StatisticsTankHitEvent& event);
	void OnTankDied(const StatisticsTankDiedEvent& event);
	void OnBrickWallDied(const BrickWallDiedEvent& event);
	void OnSteelWallDied(const SteelWallDiedEvent& event);
	void OnBonusPickup(const StatisticsBonusPickupEvent& event);
	void OnBonusDestroyed(const StatisticsBonusDestroyedEvent& event);
	void OnBonusExpired(const StatisticsBonusExpiredEvent&);

	void OnBonusSpawn(const BonusSpawnedEvent& event);
	void OnBonusHelmetPickup(const BonusHelmetAppliedEvent& event);
	void OnBonusStarPickup(const BonusStarAppliedEvent& event);
	void OnBonusCaliberPickup(const BonusCaliberAppliedEvent& event);
	void OnBonusTankPickup(const BonusTankAppliedEvent& event);

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
