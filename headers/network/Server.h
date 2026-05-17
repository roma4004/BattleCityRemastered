#pragma once

#include "NetworkCommandQueue.h"
#include "commands/Command.h"
#include "commands/CommandBatch.h"
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>

class EventSystem;

namespace network::commands
{
using boost::asio::ip::tcp;

class Session final : public std::enable_shared_from_this<Session>
{
	tcp::socket _socket;
	boost::asio::streambuf _readBuffer{};
	boost::asio::streambuf _writeBuffer{};
	std::shared_ptr<EventSystem> _events{nullptr};
	network::NetworkCommandQueue _commandQueue;

public:
	Session(tcp::socket sock, const std::shared_ptr<EventSystem>& events);

	~Session();

	void Start();

	void DoRead();

	void DoWrite(const std::string& message);

	void ProcessReceivedData(const std::string& archiveData);
	void ProcessServerCommand(const std::shared_ptr<Command>& command);
	void OnCommandBatch(const std::shared_ptr<Command>& commands);
	void OnSignalEvent(const std::shared_ptr<Command>& command);
	void OnKeyStateChange(const std::shared_ptr<Command>& command);

	[[nodiscard]] bool IsSocketOpen() const { return _socket.is_open(); }
	[[nodiscard]] network::NetworkCommandQueue& GetCommandQueue() { return _commandQueue; }
};

class Server final
{
	tcp::acceptor _acceptor;
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<std::shared_ptr<Session>> _sessions;
	std::string _name;

	std::mutex _batchWriteMutex;
	std::shared_ptr<CommandBatch> _batch{nullptr};

	std::queue<std::shared_ptr<CommandBatch>> _sendQueue;
	std::mutex _sendQueueMutex;
	std::condition_variable _sendCondition;
	std::thread _sendThread;
	bool _isRunning{false};

	void DoAccept();

public:
	Server(boost::asio::io_context& ioContext, const std::string& host, const std::string& port,
		   const std::shared_ptr<EventSystem>& events);

	~Server();

	void StartSendThread();
	void StopSendThread();

	void SendCommand(const std::shared_ptr<Command>& command);

	void Subscribe();
	void SubscribeBonus();
	void Unsubscribe() const;

	void SendToAll(const std::string& message);
	void CleanupDeadSessions();

	void ProcessNetworkCommands() const
	{
		for (const auto& session : _sessions)
		{
			if (session && session->IsSocketOpen())
			{
				session->GetCommandQueue().ProcessAll(); //TODO: refactor to session->ProcessCommandQueue()
			}
		}
	}
};
}//namespace network::commands
