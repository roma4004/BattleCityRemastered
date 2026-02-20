#pragma once

#include "commands/Command.h"
#include "commands/CommandBatch.h"
#include <condition_variable>
#include <memory>
#include <mutex>
#include <queue>
#include <string>
#include <thread>
#include <vector>
#include <boost/asio.hpp>
#include <boost/asio/ip/tcp.hpp>
#include <boost/serialization/vector.hpp> //NOTE: required for serialization ServerData

class EventSystem;

using boost::asio::ip::tcp;

struct ServerData final
{
	friend class boost::serialization::access;

	template<class Archive>
	void serialize(Archive& ar, unsigned int /*version*/);

	std::string eventName{};
};

class Session final : public std::enable_shared_from_this<Session>
{
	tcp::socket _socket;
	boost::asio::streambuf _readBuffer{};
	boost::asio::streambuf _writeBuffer{};
	std::shared_ptr<EventSystem> _events{nullptr};

public:
	Session(tcp::socket sock, const std::shared_ptr<EventSystem>& events);

	~Session();

	void Start();

	void DoRead();

	void DoWrite(const std::string& message);
};

class Server final
{
	tcp::acceptor _acceptor;
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<std::shared_ptr<Session>> _sessions;
	std::string _name;

	std::mutex _batchWriteMutex;
	std::shared_ptr<CommandBatch> _batch;

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

	void SendCommand(const std::shared_ptr<Command>& command) const;

	void Subscribe();
	void SubscribeBonus();
	void Unsubscribe() const;
	void UnsubscribeBonus() const;

	void SendToAll(const std::string& message) const;
};

// Include the template implementation
#include "Server.tpp"
