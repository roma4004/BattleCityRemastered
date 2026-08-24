#pragma once

#include "enums/DisconnectReason.h"
#include "ReplicationPublisher.h"
#include "Session.h"
#include "commands/CommandBatch.h"
#include "components/EventSystem.h"
#include <atomic>
#include <boost/asio/io_context.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <condition_variable>
#include <functional>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

class EventSystem;
struct NetworkEndFrameEvent;

namespace network::commands
{
using boost::asio::ip::tcp;

class Server final
{
public:
	Server(boost::asio::io_context& ioContext, std::string host, uint16_t port,
		   const std::shared_ptr<EventSystem>& events);

	~Server();

	void Shutdown();

	void Shutdown(DisconnectReason reason, const std::function<void()>& onClosed);

	[[nodiscard]] uint16_t GetBoundPort() const { return _acceptor.local_endpoint().port(); }

	void ProcessNetworkCommands() const;

private:
	void DoAccept();

	[[nodiscard]] std::vector<std::shared_ptr<Session>> SnapshotSessions() const;

	void StartSendThread();
	void StopSendThread();

	void SendCommand(const CommandBatch& command);

	void OnNetworkEndFrame(const NetworkEndFrameEvent&);

	void SendToAll(const std::shared_ptr<const std::string>& message);
	void CleanupDeadSessions();
	void CloseAcceptor();

	tcp::acceptor _acceptor;
	std::shared_ptr<EventSystem> _events{nullptr};
	ReplicationPublisher _replication;
	std::vector<EventSubscription> _subs{};

	std::vector<std::shared_ptr<Session>> _sessions;
	mutable std::mutex _sessionsMutex;

	std::queue<CommandBatch> _sendQueue;
	std::mutex _sendQueueMutex;
	std::condition_variable _sendCondition;
	std::thread _sendThread;
	std::atomic<bool> _isRunning{false};
};
}//namespace network::commands
