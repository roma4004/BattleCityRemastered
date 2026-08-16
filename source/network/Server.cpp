#include "network/Server.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ObstacleAndBonusEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/StatisticsEvents.h"
#include "network/commands/CommandBatch.h"
#include "utils/NetworkLogger.h"
#include <algorithm>
#include <boost/asio/strand.hpp>
#include <ser20/archives/portable_binary.hpp>
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
	});
}

Session::~Session()
{
	Shutdown();
}

void Session::Shutdown() { _channel->Close(); }

void Session::Start()
{
	//NOTE: weak, not shared - the channel stores these callbacks, so a shared_ptr would close the
	//loop Session -> channel -> callback -> Session
	const std::weak_ptr<Session> weakSelf = weak_from_this();
	_channel->SetHandlers(
			[weakSelf](const std::string& frame)
			{
				if (const auto self = weakSelf.lock())
				{
					self->_dispatcher.Dispatch(frame);
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
	const auto& cmd = std::get<SignalEvent>(command);
	const ClientSignal signal = cmd.GetSignal();

	_commandQueue.Enqueue([this, signal]()
	{
		switch (signal)
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
		 [](EventSystem& events, const std::string&, const bool pressed)
		 { events.EmitEvent(ServerInPauseReleasedEvent{.isPaused = pressed}); }},
};

void Session::OnKeyStateChange(const AnyCommand& command)
{
	const auto& cmd = std::get<KeyStateChange>(command);
	const PlayerTag tag = cmd.GetTag();
	const InputSignal action = cmd.GetAction();
	const bool isEnable = cmd.GetIsEnable();

	_commandQueue.Enqueue([this, tag, action, isEnable]()//TODO: validate each command, security risk
	{
		const auto it = kInputEmitters.find(action);
		if (it == kInputEmitters.end())
		{
			NetworkLogger::WriteError("Session::OnKeyStateChange: unhandled input signal "
									  + std::to_string(static_cast<int>(action)));
			return;
		}

		//NOTE: the local bus is still keyed by the "P1"/"P2" string, only the wire is typed
		it->second(*_events, tag == PlayerTag::P1 ? "P1" : "P2", isEnable);
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

			if (!batch.IsEmpty())
			{
				try
				{
					SendCommand(batch);
				}
				catch (const std::exception& e)
				{
					NetworkLogger::WriteError(std::string("Server send thread: ") + e.what());

					// retry send
					std::scoped_lock lock(this->_sendQueueMutex);
					this->_sendQueue.push(std::move(batch));
				}
				catch (...)
				{
					NetworkLogger::WriteError("Server send thread error: unknown exception");
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
	if (_acceptor.is_open())
	{
		boost::system::error_code ec;
		std::ignore = _acceptor.cancel(ec);
		std::ignore = _acceptor.close(ec);
	}

	for (const auto& session: SnapshotSessions())
	{
		if (session)
		{
			session->Shutdown();
		}
	}
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
	_subs.push_back(_events->AddListener(this, &Server::OnDispose));

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

void Server::OnPauseStatus(const ServerOutPauseStatusEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(KeyStateChange{PlayerTag::None, InputSignal::PauseStatus, event.isPaused});
}

void Server::OnPlayersTeamIsWon(const ServerOutPlayersTeamIsWonEvent&)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(GameStateChange{"PlayersTeamIsWon"});
}

void Server::OnEnemiesTeamIsWon(const ServerOutEnemiesTeamIsWonEvent&)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(GameStateChange{"EnemiesTeamIsWon"});
}

void Server::OnPos(const ServerOutPosEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(PositionChange{event.who, event.pos, event.dir, event.uuid});
}

void Server::OnShot(const ServerOutShotEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(TankShot{event.who, event.dir, event.bulletUuid});
}

void Server::OnHealth(const ServerOutHealthEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(HealthChange{event.who, event.health, event.uuid});
}

void Server::OnDispose(const ServerOutDisposeEvent& event)//TODO: add who,
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(Dispose{"Bullet", event.uuid});
}

void Server::OnRespawnTank(const ServerOutRespawnTankEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(RespawnTank{event.type, event.uuid, event.rect});
}

void Server::OnObstacleSpawn(const ServerOutObstacleSpawnEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(ObstacleSpawn{event.rect, event.type, event.uuid});
}

void Server::OnTankSpawnComplete(const ServerOutTankSpawnCompleteEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(TankSpawnComplete{event.uuid});
}

//NOTE: GameStatistics.cpp emits one of these 11 distinct types directly. StatisticsChange's own
//discriminator is StatisticsType (see enums/StatisticsType.h), not a free-form string, so each
//listener here just names its enum value.
void Server::SubscribeStatistics()
{
	_subs.push_back(_events->AddListener(this, &Server::OnBulletHit));
	_subs.push_back(_events->AddListener(this, &Server::OnEnemyHit));
	_subs.push_back(_events->AddListener(this, &Server::OnPlayerOneHit));
	_subs.push_back(_events->AddListener(this, &Server::OnPlayerTwoHit));
	_subs.push_back(_events->AddListener(this, &Server::OnEnemyDied));
	_subs.push_back(_events->AddListener(this, &Server::OnPlayerOneDied));
	_subs.push_back(_events->AddListener(this, &Server::OnPlayerTwoDied));
	_subs.push_back(_events->AddListener(this, &Server::OnBrickWallDied));
	_subs.push_back(_events->AddListener(this, &Server::OnSteelWallDied));
	_subs.push_back(_events->AddListener(this, &Server::OnBonusPickup));
	_subs.push_back(_events->AddListener(this, &Server::OnBonusDestroyed));
}

void Server::OnBulletHit(const ServerOutBulletHitEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(StatisticsChange{StatisticsType::BulletHit, event.author, event.fraction});
}

void Server::OnEnemyHit(const ServerOutEnemyHitEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(StatisticsChange{StatisticsType::EnemyHit, event.author, event.fraction});
}

void Server::OnPlayerOneHit(const ServerOutPlayerOneHitEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(StatisticsChange{StatisticsType::PlayerOneHit, event.author, event.fraction});
}

void Server::OnPlayerTwoHit(const ServerOutPlayerTwoHitEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(StatisticsChange{StatisticsType::PlayerTwoHit, event.author, event.fraction});
}

void Server::OnEnemyDied(const ServerOutEnemyDiedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(StatisticsChange{StatisticsType::EnemyDied, event.author, event.fraction});
}

void Server::OnPlayerOneDied(const ServerOutPlayerOneDiedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(StatisticsChange{StatisticsType::PlayerOneDied, event.author, event.fraction});
}

void Server::OnPlayerTwoDied(const ServerOutPlayerTwoDiedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(StatisticsChange{StatisticsType::PlayerTwoDied, event.author, event.fraction});
}

void Server::OnBrickWallDied(const ServerOutBrickWallDiedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(StatisticsChange{StatisticsType::BrickWallDied, event.author, event.fraction});
}

void Server::OnSteelWallDied(const ServerOutSteelWallDiedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(StatisticsChange{StatisticsType::SteelWallDied, event.author, event.fraction});
}

void Server::OnBonusPickup(const ServerOutBonusPickupEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(StatisticsChange{StatisticsType::BonusPickup, event.author, event.fraction});
}

void Server::OnBonusDestroyed(const ServerOutBonusDestroyedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(StatisticsChange{StatisticsType::BonusDestroyed, event.author, event.fraction});
}

void Server::SubscribeBonus()
{
	_subs.push_back(_events->AddListener(this, &Server::OnBonusSpawn));
	_subs.push_back(_events->AddListener(this, &Server::OnBonusDeSpawn));
	_subs.push_back(_events->AddListener(this, &Server::OnFortressChange));
	_subs.push_back(_events->AddListener(this, &Server::OnBonusHelmetPickup));
	_subs.push_back(_events->AddListener(this, &Server::OnBonusStarPickup));
	_subs.push_back(_events->AddListener(this, &Server::OnBonusCaliberPickup));
	_subs.push_back(_events->AddListener(this, &Server::OnBonusTankPickup));
}

void Server::OnBonusSpawn(const ServerOutBonusSpawnEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(BonusSpawn{event.pos, event.type, event.uuid});
}

void Server::OnBonusDeSpawn(const ServerOutBonusDeSpawnEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(BonusDeSpawn{event.uuid});
}

void Server::OnFortressChange(const ServerOutFortressChangeEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(FortressChange{event.state, event.uuid});
}

void Server::OnBonusHelmetPickup(const ServerOutBonusHelmetPickupEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(BonusStatus{event.name, BonusType::Helmet, event.isActive});
}

void Server::OnBonusStarPickup(const ServerOutBonusStarPickupEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(BonusStatus{event.author, BonusType::Star});
}

void Server::OnBonusCaliberPickup(const ServerOutBonusCaliberPickupEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(BonusStatus{event.author, BonusType::Caliber});
}

void Server::OnBonusTankPickup(const ServerOutBonusTankPickupEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.AddCommand(BonusStatus{event.author, BonusType::Tank});
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
				NetworkLogger::WriteError("Server accept: " + ec.message());
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
				NetworkLogger::WriteError(std::string("Server new session start: ") + e.what());
			}
			catch (...)
			{
				NetworkLogger::WriteError("Server new session start: unknown exception");
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
			return !session || !session->IsSocketOpen();
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
		if (session && session->IsSocketOpen())
		{
			session->ProcessCommandQueue();
		}
	}
}

void Server::SendCommand(const CommandBatch& command)
{
	std::ostringstream archiveStream;
	{
		ser20::PortableBinaryOutputArchive oa(archiveStream);
		oa(command);
	}

	// NetworkLogger::LogServerOut(command.GetClassNamesW());

	const auto& basicString = archiveStream.str();
	// NetworkLogger::WriteLog("\nraw data: " + basicString +" =", true);
	SendToAll(std::make_shared<const std::string>(network::FrameMessage(basicString)));
}
}//namespace network::commands
