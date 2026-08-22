#include "network/Server.h"
#include "enums/PlayerTag.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/StatisticsEvents.h"
#include "network/commands/CommandBatch.h"
#include "network/Serializer.h"
#include "utils/Log.h"
#include <algorithm>
#include <boost/asio/strand.hpp>
#include <mutex>

namespace network::commands
{
Session::Session(tcp::socket sock, const std::shared_ptr<EventSystem>& events)
	: _channel(std::make_shared<network::FrameChannel>(std::move(sock), "Session"))
	, _events(events)
	, _dispatcher{"Session"}
{
	RegisterCommandHandlers();
}

void Session::RegisterCommandHandlers()
{
	_dispatcher.RegisterAll({
			{CommandType::SIGNAL_EVENT, [this](const AnyCommand& cmd) { OnSignalEvent(cmd); }},
			{CommandType::KEY_STATE_CHANGE, [this](const AnyCommand& cmd) { OnKeyStateChange(cmd); }},
			{CommandType::DISCONNECT, [this](const AnyCommand& cmd) { OnDisconnect(cmd); }},
	});
}

Session::~Session()
{
	Shutdown();
}

void Session::Shutdown() { _channel->Close(); }

void Session::Shutdown(const DisconnectReason reason, std::function<void()> onClosed)
{
	if (!_channel->IsOpen())
	{
		if (onClosed)
		{
			onClosed();
		}
		return;
	}

	CommandBatch farewell;
	farewell.commands.emplace_back(Disconnect{.reason = reason});

	//NOTE: straight to the channel - Server's batch is flushed by the send thread once a frame,
	//and shutdown is exactly when that stops happening
	_channel->Send(std::make_shared<const std::string>(network::FrameMessage(network::Serialize(farewell))));
	_channel->CloseAfterFlush(std::move(onClosed));
}

void Session::OnDisconnect(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<Disconnect>(command)]()
	{
		//NOTE: on the game thread, not the network one - closing makes the session collectable, and
		//collecting it before this queue is drained would take the event with it
		_channel->Close();
		_events->EmitEvent(ServerInDisconnectEvent{.reason = cmd.reason});
	});
}

void Session::Start()
{
	//NOTE: weak, not shared - the channel stores these callbacks, so a shared_ptr would close the
	//loop Session -> channel -> callback -> Session
	const std::weak_ptr<Session> weakSelf = weak_from_this();
	_channel->SetHandlers(
			[weakSelf](const std::string& frame)
			{
				//NOTE: same reading as on the client, see HandleProtocolError. With a reason, not a
				//bare close: a plain drop sends the client reconnecting into the same mismatch.
				if (const auto self = weakSelf.lock(); self && !self->_dispatcher.Dispatch(frame))
				{
					self->Shutdown(DisconnectReason::ProtocolError, nullptr);
				}
			},
			//NOTE: closing is what makes the session collectable - Server::CleanupDeadSessions
			//goes by IsSocketOpen()
			[weakSelf]
			{
				if (const auto self = weakSelf.lock())
				{
					self->_channel->Close();
				}
			});

	_channel->StartReading();
}

void Session::OnSignalEvent(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<SignalEvent>(command)]()
	{
		switch (cmd.signal)
		{
			case ClientSignal::ReadyToPlay:
				_events->EmitEvent(ServerInClientReadyToStartGameEvent{});
				break;
		}
	});
}

//NOTE: dispatch table instead of a switch, same shape as the command handlers above
const std::unordered_map<InputSignal, Session::InputEmitter> Session::kInputEmitters{
		{InputSignal::MoveUp,
		 [](EventSystem& events, const std::string& tag, const bool pressed)
		 { events.EmitEvent(Key(tag), ServerInMoveUpEvent{.isPressed = pressed}); }},
		{InputSignal::MoveDown,
		 [](EventSystem& events, const std::string& tag, const bool pressed)
		 { events.EmitEvent(Key(tag), ServerInMoveDownEvent{.isPressed = pressed}); }},
		{InputSignal::MoveLeft,
		 [](EventSystem& events, const std::string& tag, const bool pressed)
		 { events.EmitEvent(Key(tag), ServerInMoveLeftEvent{.isPressed = pressed}); }},
		{InputSignal::MoveRight,
		 [](EventSystem& events, const std::string& tag, const bool pressed)
		 { events.EmitEvent(Key(tag), ServerInMoveRightEvent{.isPressed = pressed}); }},
		{InputSignal::Fire,
		 [](EventSystem& events, const std::string& tag, const bool pressed)
		 { events.EmitEvent(Key(tag), ServerInFireEvent{.isPressed = pressed}); }},
		{InputSignal::PauseReleased,
		 [](EventSystem& events, const std::string&, const bool)
		 { events.EmitEvent(PauseReleasedEvent{}); }},
};

void Session::OnKeyStateChange(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<KeyStateChange>(command)]()//TODO: validate each command, security risk
	{
		const auto it = kInputEmitters.find(cmd.action);
		if (it == kInputEmitters.end())
		{
			Log::Error("Session::OnKeyStateChange: unhandled input signal "
									  + std::to_string(static_cast<int>(cmd.action)));
			return;
		}

		//NOTE: the local bus is still keyed by the "P1"/"P2" string, only the wire is typed
		it->second(*_events, cmd.tag == PlayerTag::P1 ? "P1" : "P2", cmd.isPressed);
	});
}

void Session::DoWrite(std::shared_ptr<const std::string> message) { _channel->Send(std::move(message)); }

Server::Server(boost::asio::io_context& ioContext, std::string host, uint16_t port,
			   const std::shared_ptr<EventSystem>& events)
	: _acceptor{tcp::acceptor(ioContext, tcp::endpoint(boost::asio::ip::make_address(host), port))}
	, _events{events}
{
	DoAccept();
	StartSendThread();
	Subscribe();
}

//TODO: check the flow after network game choose local mode to clean all then to choose network again successful
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
	// error_log.close();
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

void Server::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &Server::OnNetworkEndFrame));
	_subs.push_back(_events->AddListener(this, &Server::OnPauseStatus));
	_subs.push_back(_events->AddListener(this, &Server::OnPlayersTeamIsWon));
	_subs.push_back(_events->AddListener(this, &Server::OnEnemiesTeamIsWon));
	_subs.push_back(_events->AddListener(this, &Server::OnPos));
	_subs.push_back(_events->AddListener(this, &Server::OnShot));
	_subs.push_back(_events->AddListener(this, &Server::OnHealth));
	_subs.push_back(_events->AddListener(this, &Server::OnDespawn));

	SubscribeStatistics();

	_subs.push_back(_events->AddListener(this, &Server::OnRespawnTank));
	_subs.push_back(_events->AddListener(this, &Server::OnObstacleSpawn));
	_subs.push_back(_events->AddListener(this, &Server::OnTankSpawnComplete));

	SubscribeBonus();
}

void Server::OnNetworkEndFrame(const NetworkEndFrameEvent&)
{
	CommandBatch batch;
	{
		std::scoped_lock lock(_batchWriteMutex);
		std::swap(batch, _batch);
	}
	{
		std::scoped_lock lock(_sendQueueMutex);
		_sendQueue.push(std::move(batch));
	}
	_sendCondition.notify_one();
}

void Server::OnPauseStatus(const PauseStatusEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(KeyStateChange{.tag = PlayerTag::None, .action = InputSignal::PauseStatus, .isPressed = event.isPaused});
}

void Server::OnPlayersTeamIsWon(const ServerOutPlayersTeamIsWonEvent&)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(GameStateChange{.gameState = "PlayersTeamIsWon"});
}

void Server::OnEnemiesTeamIsWon(const ServerOutEnemiesTeamIsWonEvent&)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(GameStateChange{.gameState = "EnemiesTeamIsWon"});
}

void Server::OnPos(const PosChangedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(PositionChange{.who = event.who, .pos = event.pos, .dir = event.dir, .uuid = event.uuid});
}

void Server::OnShot(const TankShotEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(TankShot{.who = event.who, .dir = event.dir, .uuid = event.bulletUuid});
}

void Server::OnHealth(const HealthChangedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(HealthChange{.who = event.who, .health = event.health, .uuid = event.uuid});
}

void Server::OnDespawn(const DespawnedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(Despawn{.who = event.who, .uuid = event.uuid, .reason = event.reason});
}

void Server::OnRespawnTank(const TankRespawnedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(RespawnTank{.tankType = event.type, .uuid = event.uuid, .pos = event.pos});
}

void Server::OnObstacleSpawn(const ObstacleSpawnedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(ObstacleSpawn{.pos = event.pos, .obstacleType = event.type, .uuid = event.uuid});
}

void Server::OnTankSpawnComplete(const TankSpawnCompletedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(TankSpawnComplete{.uuid = event.uuid});
}

//NOTE: GameStatistics.cpp emits one of these 11 distinct types directly. StatisticsChange's own
//discriminator is StatisticsType (see enums/StatisticsType.h), not a free-form string, so each
//listener here just names its enum value.
void Server::SubscribeStatistics()
{
	_subs.push_back(_events->AddListener(this, &Server::OnBulletHit));
	_subs.push_back(_events->AddListener(this, &Server::OnTankHit));
	_subs.push_back(_events->AddListener(this, &Server::OnTankDied));
	_subs.push_back(_events->AddListener(this, &Server::OnBrickWallDied));
	_subs.push_back(_events->AddListener(this, &Server::OnSteelWallDied));
	_subs.push_back(_events->AddListener(this, &Server::OnBonusPickup));
	_subs.push_back(_events->AddListener(this, &Server::OnBonusDestroyed));
	_subs.push_back(_events->AddListener(this, &Server::OnBonusExpired));
}

void Server::OnBulletHit(const StatisticsBulletHitEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(StatisticsChange{.statisticsType = StatisticsType::BulletHit, .author = event.author, .fraction = event.fraction});
}

void Server::OnTankHit(const StatisticsTankHitEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(StatisticsChange{.statisticsType = StatisticsType::TankHit, .who = event.who, .author = event.author, .fraction = event.fraction});
}

void Server::OnTankDied(const StatisticsTankDiedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(StatisticsChange{.statisticsType = StatisticsType::TankDied, .who = event.who, .author = event.author, .fraction = event.fraction});
}

void Server::OnBrickWallDied(const BrickWallDiedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(StatisticsChange{.statisticsType = StatisticsType::BrickWallDied, .author = event.author, .fraction = event.fraction});
}

void Server::OnSteelWallDied(const SteelWallDiedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(StatisticsChange{.statisticsType = StatisticsType::SteelWallDied, .author = event.author, .fraction = event.fraction});
}

void Server::OnBonusPickup(const StatisticsBonusPickupEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(StatisticsChange{.statisticsType = StatisticsType::BonusPickup, .author = event.author, .fraction = event.fraction});
}

void Server::OnBonusDestroyed(const StatisticsBonusDestroyedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(StatisticsChange{.statisticsType = StatisticsType::BonusDestroyed, .author = event.author, .fraction = event.fraction});
}

void Server::OnBonusExpired(const StatisticsBonusExpiredEvent&)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(StatisticsChange{.statisticsType = StatisticsType::BonusExpired});
}

void Server::SubscribeBonus()
{
	_subs.push_back(_events->AddListener(this, &Server::OnBonusSpawn));
	_subs.push_back(_events->AddListener(this, &Server::OnFortressChange));
	_subs.push_back(_events->AddListener(this, &Server::OnBonusHelmetPickup));
	_subs.push_back(_events->AddListener(this, &Server::OnBonusStarPickup));
	_subs.push_back(_events->AddListener(this, &Server::OnBonusCaliberPickup));
	_subs.push_back(_events->AddListener(this, &Server::OnBonusTankPickup));
}

void Server::OnBonusSpawn(const BonusSpawnedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(BonusSpawn{.pos = event.pos, .bonusType = event.type, .uuid = event.uuid});
}

void Server::OnFortressChange(const FortressChangedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(FortressChange{.state = event.state, .uuid = event.uuid});
}

void Server::OnBonusHelmetPickup(const BonusHelmetAppliedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(BonusStatus{.name = event.name, .bonusType = BonusType::Helmet, .isEnable = event.isActive});
}

void Server::OnBonusStarPickup(const BonusStarAppliedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(BonusStatus{.name = event.name, .bonusType = BonusType::Star});
}

void Server::OnBonusCaliberPickup(const BonusCaliberAppliedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(BonusStatus{.name = event.name, .bonusType = BonusType::Caliber});
}

void Server::OnBonusTankPickup(const BonusTankAppliedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(BonusStatus{.name = event.name, .bonusType = BonusType::Tank});
}

void Server::DoAccept()
{
	//NOTE: own strand per socket - serialises that session's handlers against each other
	_acceptor.async_accept(boost::asio::make_strand(_acceptor.get_executor()),
						   [this](const boost::system::error_code& ec, tcp::socket socket)
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
				//TODO: add feature to restart game with existing session
				auto session = std::make_shared<Session>(std::move(socket), _events);
				{
					std::scoped_lock lock(_sessionsMutex);
					_sessions.emplace_back(session);
				}
				// _events->EmitEvent("NewClientConnected");
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
