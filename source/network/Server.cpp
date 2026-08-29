#include "network/Server.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "network/commands/CommandBatch.h"
#include "network/Serializer.h"
#include "utils/Log.h"
#include <algorithm>
#include <boost/asio/strand.hpp>
#include <mutex>

namespace network::commands
{
Server::Server(boost::asio::io_context& ioContext, std::string host, uint16_t port,
			   const std::shared_ptr<EventSystem>& events)
	: _acceptor{tcp::acceptor(ioContext, tcp::endpoint(boost::asio::ip::make_address(host), port))}
	, _events{events}
	, _replication{events}
{
	DoAccept();
	StartSendThread();
	_subs.push_back(_events->AddListener(this, &Server::OnNetworkEndFrame));
}

void Server::StartSendThread()
{
	_isRunning.store(true);
	_sendThread = std::thread([this]()
	{
		while (this->_isRunning.load())
		{
			CommandBatch batch;
			{
				std::unique_lock<std::mutex> lock(this->_sendQueueMutex);
				this->_sendCondition.wait(
						lock, [this] { return !this->_sendQueue.empty() || !this->_isRunning.load(); });

				if (!this->_isRunning.load())
					break;

				if (this->_sendQueue.empty())
					continue;

				batch = std::move(this->_sendQueue.front());
				this->_sendQueue.pop();
			}

			if (!batch.commands.empty())
			{
				try
				{
					SendCommand(batch);
				}
				catch (const std::exception& e)
				{
					Log::Error(std::string("Server send thread: ") + e.what());

					// retry send
					std::scoped_lock lock(this->_sendQueueMutex);
					this->_sendQueue.push(std::move(batch));
				}
				catch (...)
				{
					Log::Error("Server send thread error: unknown exception");
				}
			}
		}
	});
}

void Server::StopSendThread()
{
	{
		std::scoped_lock lock(_sendQueueMutex);
		_isRunning.store(false);
	}

	_sendCondition.notify_one();

	if (_sendThread.joinable())
	{
		_sendThread.join();
	}
}

Server::~Server()
{
	StopSendThread();
	Shutdown();
}

void Server::Shutdown()
{
	CloseAcceptor();

	for (const auto& session: SnapshotSessions())
	{
		if (session)
		{
			session->Shutdown();
		}
	}
}

void Server::Shutdown(const DisconnectReason reason, const std::function<void()>& onClosed)
{
	CloseAcceptor();

	const auto sessions = SnapshotSessions();
	//NOTE: counted, not per-session - the caller is told once, after the last goodbye is out
	const auto pending = std::make_shared<std::size_t>(sessions.size());

	if (sessions.empty())
	{
		if (onClosed)
		{
			onClosed();
		}
		return;
	}

	for (const auto& session: sessions)
	{
		if (!session)
		{
			--*pending;
			continue;
		}

		session->Shutdown(reason, [pending, onClosed]
		{
			if (--*pending == 0u && onClosed)
			{
				onClosed();
			}
		});
	}
}

void Server::CloseAcceptor()
{
	if (!_acceptor.is_open())
	{
		return;
	}

	boost::system::error_code ec;
	std::ignore = _acceptor.cancel(ec);
	std::ignore = _acceptor.close(ec);
}

void Server::OnNetworkEndFrame(const NetworkEndFrameEvent&)
{
	{
		std::scoped_lock lock(_sendQueueMutex);
		_sendQueue.push(_replication.TakeBatch());
	}
	_sendCondition.notify_one();
}

void Server::DoAccept()
{
	const auto executor = boost::asio::make_strand(_acceptor.get_executor());
	//NOTE: own strand per socket - serializes that session's handlers against each other
	_acceptor.async_accept(executor, [this](const boost::system::error_code& ec, tcp::socket socket)
	{
		if (ec)
		{
			if (ec != boost::asio::error::operation_aborted)
			{
				Log::Error("Server accept: " + ec.message());
			}
		}
		else
		{
			try
			{
				auto session = std::make_shared<Session>(std::move(socket), _events);
				{
					std::scoped_lock lock(_sessionsMutex);
					_sessions.emplace_back(session);
				}
				session->Start();//NOTE: outside the lock - it posts reads and can reach the event bus
			}
			catch (const std::exception& e)
			{
				Log::Error(std::string("Server new session start: ") + e.what());
			}
			catch (...)
			{
				Log::Error("Server new session start: unknown exception");
			}
			DoAccept();
		}
	});
}

std::vector<std::shared_ptr<Session>> Server::SnapshotSessions() const
{
	std::scoped_lock lock(_sessionsMutex);
	return _sessions;
}

void Server::CleanupDeadSessions()
{
	std::vector<std::shared_ptr<Session>> dead;//NOTE: ~Session closes a socket - not under the lock
	{
		std::scoped_lock lock(_sessionsMutex);
		auto removed = std::ranges::remove_if(_sessions, [](const std::shared_ptr<Session>& session)
		{
			//NOTE: a closed session with commands still queued is not dead yet - its goodbye is unread
			return !session || (!session->IsSocketOpen() && !session->HasPendingCommands());
		});
		dead.insert(dead.end(), std::make_move_iterator(removed.begin()), std::make_move_iterator(removed.end()));
		_sessions.erase(removed.begin(), removed.end());
	}
}

void Server::SendToAll(const std::shared_ptr<const std::string>& message)
{
	CleanupDeadSessions();

	for (const auto& session: SnapshotSessions())
	{
		if (session && session->IsSocketOpen())
		{
			session->DoWrite(message);
		}
	}
}

void Server::ProcessNetworkCommands() const
{
	for (const auto& session: SnapshotSessions())
	{
		//NOTE: no IsSocketOpen check - frames already read stay valid, and the last is the goodbye
		if (session)
		{
			session->ProcessCommandQueue();
		}
	}
}

void Server::SendCommand(const CommandBatch& command)
{

	const std::string basicString = network::Serialize(command);
	SendToAll(std::make_shared<const std::string>(network::FrameMessage(basicString)));
}
}//namespace network::commands
