#include "network/Client.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ObstacleAndBonusEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/StatisticsEvents.h"
#include "geometry/ObjRectangle.h"
#include "enums/CommandType.h"
#include "enums/FortressState.h"
#include "enums/StatisticsType.h"
#include "network/commands/CommandBatch.h"
#include "network/Serializer.h"
#include "utils/Log.h"
#include <cassert>
#include <string>
#include <tuple>

namespace network::commands
{
Client::Client(boost::asio::io_context& ioContext, std::string host, uint16_t port,
			   const std::shared_ptr<EventSystem>& events)
	: _strand(boost::asio::make_strand(ioContext))
	, _channel(std::make_shared<network::FrameChannel>(tcp::socket(_strand), "Client"))
	, _reconnectTimer(_strand)
	, _endpoint{tcp::endpoint(boost::asio::ip::make_address(host), port)}
	, _events{events}
	, _dispatcher{"Client"}
{
	Subscribe();
	RegisterCommandHandlers();

	TryConnect();
}

void Client::RegisterCommandHandlers()
{
	_dispatcher.RegisterAll({
			{CommandType::POSITION_CHANGE, [this](const AnyCommand& cmd) { OnPositionChange(cmd); }},
			{CommandType::TANK_SHOT, [this](const AnyCommand& cmd) { OnTankShot(cmd); }},
			{CommandType::HEALTH_CHANGE, [this](const AnyCommand& cmd) { OnHealthChange(cmd); }},
			{CommandType::DISPOSE, [this](const AnyCommand& cmd) { OnDispose(cmd); }},
			{CommandType::STATISTICS_CHANGE, [this](const AnyCommand& cmd) { OnStatisticsChange(cmd); }},
			{CommandType::KEY_STATE_CHANGE, [this](const AnyCommand& cmd) { OnKeyStateChange(cmd); }},
			{CommandType::GAME_STATE_CHANGE, [this](const AnyCommand& cmd) { OnGameStateChange(cmd); }},
			{CommandType::FORTRESS_CHANGE, [this](const AnyCommand& cmd) { OnFortressChange(cmd); }},
			{CommandType::BONUS_SPAWN, [this](const AnyCommand& cmd) { OnBonusSpawn(cmd); }},
			{CommandType::BONUS_DESPAWN, [this](const AnyCommand& cmd) { OnBonusDeSpawn(cmd); }},
			{CommandType::RESPAWN_TANK, [this](const AnyCommand& cmd) { OnRespawnTank(cmd); }},
			{CommandType::OBSTACLE_SPAWN, [this](const AnyCommand& cmd) { OnObstacleSpawn(cmd); }},
			{CommandType::TANK_SPAWN_COMPLETE, [this](const AnyCommand& cmd) { OnTankSpawnComplete(cmd); }},
			{CommandType::BONUS_STATUS, [this](const AnyCommand& cmd) { OnBonusStatus(cmd); }},
			{CommandType::DISCONNECT, [this](const AnyCommand& cmd) { OnDisconnect(cmd); }},
	});
}

//TODO: fix reconnect for minGW when we too fast run host and client
void Client::TryConnect()
{
	auto& socket = _channel->Socket();
	if (socket.is_open())
	{
		boost::system::error_code ec;
		std::ignore = socket.close(ec);// NOTE: error captured via ec, return value intentionally discarded
	}

	socket.open(_endpoint.protocol());
	socket.async_connect(_endpoint, [this](const boost::system::error_code& ec)
	{
		if (!ec)
		{
			Log::Info("client connected");
			_reconnectAttempts = 0;
			_isConnected = true;
			this->StartReading();
			_channel->SetWriteEnabled(true);
			{
				std::scoped_lock lock(_batchWriteMutex);
				this->_batch.commands.emplace_back(SignalEvent{.signal = ClientSignal::ReadyToPlay});
			}
		}
		else
		{
			++_reconnectAttempts;
			Log::Error("Client connect failed (attempt " + std::to_string(_reconnectAttempts) + "/"
										  + std::to_string(MaxReconnectAttempts) + "): " + ec.message());

			if (_reconnectAttempts < MaxReconnectAttempts)
			{
				ScheduleReconnect();
			}
			else
			{
				Log::Error("Client gave up after " + std::to_string(MaxReconnectAttempts) + " attempts");
			}
		}
	});
}

void Client::ScheduleReconnect()
{
	if (_reconnectPending || _isShuttingDown)
	{
		return;
	}

	_reconnectPending = true;

	//NOTE: weak for the same reason as in StartReading - the timer is our own member, so a shared
	//capture would keep this Client alive through its own pending handler
	const std::weak_ptr<Client> weakSelf = weak_from_this();
	_reconnectTimer.expires_after(std::chrono::milliseconds(ReconnectDelayMs));
	_reconnectTimer.async_wait([weakSelf](const boost::system::error_code& timerEc)
	{
		const auto self = weakSelf.lock();
		if (!self)
		{
			return;
		}

		self->_reconnectPending = false;
		if (!timerEc && !self->_isShuttingDown)
		{
			self->TryConnect();
		}
	});
}

void Client::HandleDisconnect()
{
	if (_isShuttingDown || _reconnectPending)
	{
		return;
	}

	//NOTE: the expected tail of an announced leave - reconnecting would hammer a closing port
	if (_isHostGone)
	{
		_isConnected = false;
		_channel->SetWriteEnabled(false);
		_channel->Close();
		return;
	}

	_isConnected = false;
	_channel->SetWriteEnabled(false);
	_channel->ResetWriteState();

	if (_channel->IsOpen())
	{
		boost::system::error_code ec;
		std::ignore = _channel->Socket().close(ec);
	}

	_reconnectAttempts = 0;//NOTE: a drop starts a fresh budget, it is not a failed connect attempt
	ScheduleReconnect();
}

//NOTE: TCP hands bytes over intact or not at all, so an unreadable frame is a protocol
//disagreement, not line noise - the next one fails the same way and a reconnect reaches the
//same host. Hence: end the link, and tell the game side why.
void Client::HandleProtocolError()
{
	if (_isShuttingDown)
	{
		return;
	}

	_isConnected = false;
	_commandQueue.Enqueue([this]
	{
		_events->EmitEvent(ClientInDisconnectEvent{.reason = DisconnectReason::ProtocolError});
	});

	Shutdown();
}

Client::~Client()
{
	Shutdown();
}

void Client::Shutdown()
{
	_isShuttingDown = true;//NOTE: before cancelling - handlers must not read the cancel as a drop

	std::ignore = _reconnectTimer.cancel();//NOTE: no-throw, so ~Client is safe without a catch-all
	_channel->Close();
}

void Client::Shutdown(const DisconnectReason reason, std::function<void()> onClosed)
{
	_isShuttingDown = true;
	std::ignore = _reconnectTimer.cancel();

	//NOTE: nobody to tell - there is no link, so this degrades to the plain shutdown above
	if (!_isConnected)
	{
		_channel->Close();
		if (onClosed)
		{
			onClosed();
		}
		return;
	}

	CommandBatch farewell;
	farewell.commands.emplace_back(Disconnect{.reason = reason});
	SendCommand(farewell);

	_isConnected = false;
	_channel->CloseAfterFlush(std::move(onClosed));
}

void Client::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &Client::OnNetworkEndFrame));

	//NOTE: local dispatch is keyed by the "P2" string; on the wire the tag is PlayerTag::P2
	_subs.push_back(_events->AddListener(Key(std::string{"P2"}), this, &Client::OnMoveUp));
	_subs.push_back(_events->AddListener(Key(std::string{"P2"}), this, &Client::OnMoveLeft));
	_subs.push_back(_events->AddListener(Key(std::string{"P2"}), this, &Client::OnMoveDown));
	_subs.push_back(_events->AddListener(Key(std::string{"P2"}), this, &Client::OnMoveRight));
	_subs.push_back(_events->AddListener(Key(std::string{"P2"}), this, &Client::OnFire));

	_subs.push_back(_events->AddListener(this, &Client::OnClientOutReadyToPlay));
	_subs.push_back(_events->AddListener(this, &Client::OnClientOutPauseStatus));
}

void Client::OnNetworkEndFrame(const NetworkEndFrameEvent&)
{
	CommandBatch batch;
	{
		std::scoped_lock lock(_batchWriteMutex);
		std::swap(batch, _batch);
	}

	if (!batch.commands.empty())
	{
		SendCommand(batch);
	}
}

void Client::OnMoveUp(const MoveUpEvent& event) { SendKeyState(InputSignal::MoveUp, event.isPressed); }
void Client::OnMoveLeft(const MoveLeftEvent& event) { SendKeyState(InputSignal::MoveLeft, event.isPressed); }
void Client::OnMoveDown(const MoveDownEvent& event) { SendKeyState(InputSignal::MoveDown, event.isPressed); }
void Client::OnMoveRight(const MoveRightEvent& event) { SendKeyState(InputSignal::MoveRight, event.isPressed); }
void Client::OnFire(const FireEvent& event) { SendKeyState(InputSignal::Fire, event.isPressed); }

void Client::OnClientOutReadyToPlay(const ClientOutReadyToPlayEvent&)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(SignalEvent{.signal = ClientSignal::ReadyToPlay});
}

void Client::OnClientOutPauseStatus(const ClientOutPauseStatusEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(KeyStateChange{.tag = PlayerTag::None, .action = InputSignal::PauseReleased, .isPressed = event.isPaused});
}

void Client::StartReading()
{
	//NOTE: weak, not shared - the channel outlives nothing here, but it *stores* these callbacks,
	//so capturing a shared_ptr would close the loop Client -> channel -> callback -> Client
	const std::weak_ptr<Client> weakSelf = weak_from_this();
	_channel->SetHandlers(
			[weakSelf](const std::string& frame)
			{
				if (const auto self = weakSelf.lock(); self && !self->_dispatcher.Dispatch(frame))
				{
					self->HandleProtocolError();
				}
			},
			[weakSelf]
			{
				if (const auto self = weakSelf.lock())
				{
					self->HandleDisconnect();
				}
			});

	_channel->StartReading();
}

void Client::SendKeyState(const InputSignal action, const bool state)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(KeyStateChange{.tag = PlayerTag::P2, .action = action, .isPressed = state});
}

void Client::OnPositionChange(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<PositionChange>(command)]()
	{
		_events->EmitEvent(Key(cmd.uuid), ClientInPosEvent{.pos = cmd.pos, .dir = cmd.dir});
	});
}

void Client::OnTankShot(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<TankShot>(command)]()
	{
		_events->EmitEvent(Key(cmd.who), ClientInShotEvent{.dir = cmd.dir, .bulletUuid = cmd.uuid});
	});
}

void Client::OnHealthChange(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<HealthChange>(command)]()
	{
		_events->EmitEvent(Key(cmd.uuid), ClientInHealthEvent{.health = cmd.health});
	});
}

void Client::OnDispose(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<Dispose>(command)]()
	{
		_events->EmitEvent(Key(cmd.uuid), ClientInDisposeEvent{});
	});
}

void Client::OnStatisticsChange(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<StatisticsChange>(command)]()
	{
		switch (cmd.statisticsType)
		{
			case StatisticsType::BulletHit:
				_events->EmitEvent(ClientInBulletHitEvent{.author = cmd.author, .fraction = cmd.fraction});
				break;
			case StatisticsType::EnemyHit:
				_events->EmitEvent(ClientInEnemyHitEvent{.author = cmd.author, .fraction = cmd.fraction});
				break;
			case StatisticsType::PlayerOneHit:
				_events->EmitEvent(ClientInPlayerOneHitEvent{.author = cmd.author, .fraction = cmd.fraction});
				break;
			case StatisticsType::PlayerTwoHit:
				_events->EmitEvent(ClientInPlayerTwoHitEvent{.author = cmd.author, .fraction = cmd.fraction});
				break;
			case StatisticsType::EnemyDied:
				_events->EmitEvent(ClientInEnemyDiedEvent{.author = cmd.author, .fraction = cmd.fraction});
				break;
			case StatisticsType::PlayerOneDied:
				_events->EmitEvent(ClientInPlayerOneDiedEvent{.author = cmd.author, .fraction = cmd.fraction});
				break;
			case StatisticsType::PlayerTwoDied:
				_events->EmitEvent(ClientInPlayerTwoDiedEvent{.author = cmd.author, .fraction = cmd.fraction});
				break;
			case StatisticsType::BrickWallDied:
				_events->EmitEvent(ClientInBrickWallDiedEvent{.author = cmd.author, .fraction = cmd.fraction});
				break;
			case StatisticsType::SteelWallDied:
				_events->EmitEvent(ClientInSteelWallDiedEvent{.author = cmd.author, .fraction = cmd.fraction});
				break;
			case StatisticsType::BonusPickup:
				_events->EmitEvent(ClientInBonusPickupEvent{.author = cmd.author, .fraction = cmd.fraction});
				break;
			case StatisticsType::BonusDestroyed:
				_events->EmitEvent(ClientInBonusDestroyedEvent{.author = cmd.author, .fraction = cmd.fraction});
				break;
		}
	});
}

void Client::OnKeyStateChange(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<KeyStateChange>(command)]()
	{
		if (cmd.action == InputSignal::PauseStatus)
		{
			_events->EmitEvent(PauseStatusEvent{.isPaused = cmd.isPressed});
			return;
		}

		Log::Info("Client::OnKeyStateChange: unexpected signal "
								+ std::to_string(static_cast<int>(cmd.action)));
	});
}

void Client::OnGameStateChange(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<GameStateChange>(command)]()
	{
		if (cmd.gameState == "PlayersTeamIsWon")
		{
			this->_events->EmitEvent(PlayersTeamIsWonEvent{});
		}
		else if (cmd.gameState == "EnemiesTeamIsWon")
		{
			this->_events->EmitEvent(EnemiesTeamIsWonEvent{});
		}
		else
		{
			Log::Info("Client::OnGameStateChange: unrecognized game state \"" + cmd.gameState + "\"");
		}
	});
}

void Client::OnFortressChange(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<FortressChange>(command)]()
	{
		//NOTE: no default - the compiler flags an unhandled state, and only an off-enum value from the
		//wire reaches past the switch
		switch (cmd.state)
		{
			case FortressState::Died:
				_events->EmitEvent(Key(cmd.uuid), ClientInFortressDiedEvent{});
				return;
			case FortressState::ToBrick:
				_events->EmitEvent(Key(cmd.uuid), ClientInFortressToBrickEvent{});
				return;
			case FortressState::ToSteel:
				_events->EmitEvent(Key(cmd.uuid), ClientInFortressToSteelEvent{});
				return;
		}

		Log::Error("Client::OnFortressChange: unknown state "
								  + std::to_string(static_cast<int>(cmd.state)));
	});
}

void Client::OnBonusSpawn(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<BonusSpawn>(command)]()
	{
		_events->EmitEvent(ClientInBonusSpawnEvent{.pos = cmd.pos, .type = cmd.bonusType, .uuid = cmd.uuid});
	});
}

void Client::OnBonusDeSpawn(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<BonusDeSpawn>(command)]()
	{
		_events->EmitEvent(ClientInBonusDeSpawnEvent{.uuid = cmd.uuid});
	});
}

void Client::OnRespawnTank(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<RespawnTank>(command)]()
	{
		_events->EmitEvent(ClientInRespawnTankEvent{.type = cmd.tankType, .uuid = cmd.uuid, .rect = cmd.rect});
	});
}

void Client::OnObstacleSpawn(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<ObstacleSpawn>(command)]()
	{
		_events->EmitEvent(ClientInObstacleSpawnEvent{.rect = cmd.rect, .type = cmd.obstacleType, .uuid = cmd.uuid});
	});
}

void Client::OnTankSpawnComplete(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<TankSpawnComplete>(command)]()
	{
		_events->EmitEvent(ClientInTankSpawnCompleteEvent{.uuid = cmd.uuid});
	});
}

void Client::OnBonusStatus(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<BonusStatus>(command)]()
	{
		switch (cmd.bonusType)
		{
			case BonusType::Helmet:
				_events->EmitEvent(Key(cmd.name), ClientInBonusHelmetPickupEvent{.isEnable = cmd.isEnable});
				break;
			case BonusType::Star:
				_events->EmitEvent(Key(cmd.name), ClientInBonusStarPickupEvent{});
				break;
			case BonusType::Caliber:
				_events->EmitEvent(Key(cmd.name), ClientInBonusCaliberPickupEvent{});
				break;
			case BonusType::Tank:
				_events->EmitEvent(ClientInBonusTankPickupEvent{.name = cmd.name});
				break;
			default:
				//NOTE: Server only ever constructs BonusStatus with Helmet/Star/Caliber/Tank
				//(see Server.cpp), so reaching here means a new BonusType wasn't wired up above
				assert(false && "Client::OnBonusStatus: unhandled BonusType");
				break;
		}
	});
}

void Client::OnDisconnect(const AnyCommand& command)
{
	const auto reason = std::get<Disconnect>(command).reason;

	//NOTE: on the network thread, not in the queued lambda - the EOF arrives well before the game
	//thread drains the queue, and HandleDisconnect must already know why
	_isHostGone = true;

	_commandQueue.Enqueue([this, reason]()
	{
		_events->EmitEvent(ClientInDisconnectEvent{.reason = reason});
	});
}

void Client::SendCommand(const CommandBatch& command)
{
	_channel->Send(std::make_shared<const std::string>(network::FrameMessage(network::Serialize(command))));
}
}//namespace network::commands
