#pragma once

#include "NetworkCommandQueue.h"
#include "commands/Command.h"
#include "commands/CommandBatch.h"
#include "components/EventSystem.h"
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <atomic>
#include <unordered_map>
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

	[[nodiscard]] bool IsSocketOpen() const { return _socket.is_open(); }
	[[nodiscard]] network::NetworkCommandQueue& GetCommandQueue() { return _commandQueue; }

	void Start();
	void DoWrite(const std::string& message);
	void Shutdown();

private:
	using CommandHandler = std::function<void(const std::shared_ptr<Command>&)>;

	void DoRead();

	void ProcessReceivedData(const std::string& archiveData);
	void ProcessServerCommand(const std::shared_ptr<Command>& command);
	void RegisterCommandHandlers();
	void OnCommandBatch(const std::shared_ptr<Command>& commands);
	void OnSignalEvent(const std::shared_ptr<Command>& command);
	void OnKeyStateChange(const std::shared_ptr<Command>& command);

	tcp::socket _socket;
	boost::asio::streambuf _readBuffer{};
	boost::asio::streambuf _writeBuffer{};
	std::shared_ptr<EventSystem> _events{nullptr};
	network::NetworkCommandQueue _commandQueue;
	std::unordered_map<CommandType, CommandHandler> _commandHandlers{};

};

class Server final
{
public:
	Server(boost::asio::io_context& ioContext, std::string host, uint16_t port,
		   const std::shared_ptr<EventSystem>& events);

	~Server();

	void Shutdown();

	[[nodiscard]] uint16_t GetBoundPort() const { return _acceptor.local_endpoint().port(); }

	void ProcessNetworkCommands() const
	{
		for (const auto& session: _sessions)
		{
			if (session && session->IsSocketOpen())
			{
				session->GetCommandQueue().ProcessAll();//TODO: refactor to session->ProcessCommandQueue()
			}
		}
	}

private:
	void DoAccept();

	void StartSendThread();
	void StopSendThread();

	void SendCommand(const std::shared_ptr<Command>& command);

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

	void SendToAll(const std::string& message);
	void CleanupDeadSessions();

	tcp::acceptor _acceptor;
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};
	std::vector<std::shared_ptr<Session>> _sessions;
	std::string _name;

	std::mutex _batchWriteMutex;
	std::shared_ptr<CommandBatch> _batch{nullptr};

	std::queue<std::shared_ptr<CommandBatch>> _sendQueue;
	std::mutex _sendQueueMutex;
	std::condition_variable _sendCondition;
	std::thread _sendThread;
	std::atomic<bool> _isRunning{false};
};
}//namespace network::commands
