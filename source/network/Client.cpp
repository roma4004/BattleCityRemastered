#include "network/Client.h"
#include "components/EventSystem.h"
#include "components/events/SpawnEvents.h"
#include "components/events/BonusPickupEvents.h"
#include "components/events/CoreLifecycleEvents.h"
#include "components/events/InputEvents.h"
#include "components/events/ObjectLifecycleEvents.h"
#include "components/events/ReplicationEvents.h"
#include "components/events/StatisticsEvents.h"
#include "enums/CommandType.h"
#include "enums/InputChannel.h"
#include "enums/StatisticsType.h"
#include "network/commands/CommandBatch.h"
#include "network/Serializer.h"
#include "utils/Log.h"
#include <string>
#include <tuple>

namespace network::commands
{
Client::Client(boost::asio::io_context& ioContext, std::string host, const uint16_t port,
			   const std::shared_ptr<EventSystem>& events)
	: PeerLink(tcp::socket(boost::asio::make_strand(ioContext)), "Client", events)
	, _reconnectTimer(_channel->Socket().get_executor())
	, _endpoint{tcp::endpoint(boost::asio::ip::make_address(host), port)}
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
			{CommandType::DESPAWN, [this](const AnyCommand& cmd) { OnDespawn(cmd); }},
			{CommandType::STATISTICS_CHANGE, [this](const AnyCommand& cmd) { OnStatisticsChange(cmd); }},
			{CommandType::KEY_STATE_CHANGE, [this](const AnyCommand& cmd) { OnKeyStateChange(cmd); }},
			{CommandType::GAME_STATE_CHANGE, [this](const AnyCommand& cmd) { OnGameStateChange(cmd); }},
			{CommandType::BONUS_SPAWN, [this](const AnyCommand& cmd) { OnBonusSpawn(cmd); }},
			{CommandType::RESPAWN_TANK, [this](const AnyCommand& cmd) { OnRespawnTank(cmd); }},
			{CommandType::OBSTACLE_SPAWN, [this](const AnyCommand& cmd) { OnObstacleSpawn(cmd); }},
			{CommandType::TANK_SPAWN_COMPLETE, [this](const AnyCommand& cmd) { OnTankSpawnComplete(cmd); }},
			{CommandType::BONUS_SPAWN_COMPLETE, [this](const AnyCommand& cmd) { OnBonusSpawnComplete(cmd); }},
			{CommandType::TIER_CHANGE, [this](const AnyCommand& cmd) { OnTierChange(cmd); }},
			{CommandType::BONUS_STATUS, [this](const AnyCommand& cmd) { OnBonusStatus(cmd); }},
			{CommandType::DISCONNECT, [this](const AnyCommand& cmd) { OnDisconnect(cmd); }},
	});
}

void Client::TryConnect()
{
	auto& socket = _channel->Socket();
	_channel->CloseForReconnect();
	socket.open(_endpoint.protocol());
	socket.async_connect(_endpoint, [this](const boost::system::error_code& ec)
	{
		if (!ec)
		{
			Log::Info("client connected");
			_reconnectAttempts = 0;
			_reconnectAbandoned = false;
			_isConnected = true;
			_commandQueue.Enqueue([this] { _events->EmitEvent(ClientConnectedToHostEvent{}); });
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
					   + std::to_string(kMaxReconnectAttempts) + "): " + ec.message());

			ScheduleReconnect();
		}
	});
}

void Client::ScheduleReconnect()
{
	if (_reconnectPending || _isShuttingDown)
	{
		return;
	}

	if (_isLinkUnrecoverable || _reconnectAttempts >= kMaxReconnectAttempts)
	{
		if (!_reconnectAbandoned)
		{
			_reconnectAbandoned = true;
			Log::Error("Client gave up on the host");
			_commandQueue.Enqueue([this] { _events->EmitEvent(ClientReconnectAbandonedEvent{}); });
		}
		return;
	}

	_reconnectPending = true;

	//NOTE: weak for the same reason as in StartReading - the timer is our own member, so a shared
	//capture would keep this Client alive through its own pending handler
	const std::weak_ptr<Client> weakSelf = weak_from_this();
	_reconnectTimer.expires_after(std::chrono::milliseconds(kReconnectDelayMs));
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

	_isConnected = false;
	_channel->SetWriteEnabled(false);

	if (_isLinkUnrecoverable)
	{
		_channel->Close();
	}
	else
	{
		_channel->CloseForReconnect();
		_reconnectAttempts = 0;//NOTE: a drop starts a fresh budget, it is not a failed connect attempt
	}

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

	const bool hasLink = _isConnected;
	_isConnected = false;

	CloseWithFarewell(hasLink, reason, std::move(onClosed));
}

void Client::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &Client::OnNetworkEndFrame));

	//NOTE: a keyboard half belongs to a seat, not to a machine - this process is player two
	//(SendKeyState tags every press P2), so it takes the arrows like a second player anywhere else.
	//Tab swaps the halves locally for whoever would rather drive that seat with WASD
	constexpr InputChannel channel{InputChannel::LocalP2};
	_subs.push_back(_events->AddListener(Key(channel), this, &Client::OnMoveUp));
	_subs.push_back(_events->AddListener(Key(channel), this, &Client::OnMoveLeft));
	_subs.push_back(_events->AddListener(Key(channel), this, &Client::OnMoveDown));
	_subs.push_back(_events->AddListener(Key(channel), this, &Client::OnMoveRight));
	_subs.push_back(_events->AddListener(Key(channel), this, &Client::OnFire));

	_subs.push_back(_events->AddListener(this, &Client::OnClientOutReadyToPlay));
	_subs.push_back(_events->AddListener(this, &Client::OnPauseRequested));
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
		SendBatch(batch);
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

void Client::OnPauseRequested(const PauseRequestedEvent& event)
{
	std::scoped_lock lock(_batchWriteMutex);
	_batch.commands.emplace_back(KeyStateChange{.tag = PlayerTag::None,
												.action = InputSignal::PauseReleased,
												.isPressed = event.isPaused});
}

void Client::StartReading()
{
	//NOTE: weak, not shared - the channel outlives nothing here, but it *stores* these callbacks,
	//so capturing a shared_ptr would close the loop Client -> channel -> callback -> Client
	const std::weak_ptr<Client> weakSelf = weak_from_this();
	_channel->SetHandlers(
			[weakSelf](const std::string& frame)
			{
				if (const auto self = weakSelf.lock(); self && !self->DispatchFrame(frame))
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
		_events->EmitEvent(Key(cmd.uuid),
						   PosChangedEvent{.who = cmd.who, .pos = cmd.pos, .dir = cmd.dir, .uuid = cmd.uuid});
	});
}

void Client::OnTankShot(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<TankShot>(command)]()
	{
		_events->EmitEvent(Key(cmd.who), TankShotEvent{.who = cmd.who, .dir = cmd.dir, .bulletUuid = cmd.uuid});
	});
}

void Client::OnHealthChange(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<HealthChange>(command)]()
	{
		_events->EmitEvent(Key(cmd.uuid), HealthChangedEvent{.who = cmd.who, .health = cmd.health, .uuid = cmd.uuid});
	});
}

void Client::OnDespawn(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<Despawn>(command)]()
	{
		_events->EmitEvent(Key(cmd.uuid), DespawnedEvent{.who = cmd.who, .uuid = cmd.uuid, .reason = cmd.reason});
	});
}

void Client::OnStatisticsChange(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<StatisticsChange>(command)]()
	{
		const Author who = SeatFromWire(cmd.who);
		const Author author = SeatFromWire(cmd.author);

		switch (cmd.statisticsType)
		{
			case StatisticsType::BulletHit:
				_events->EmitEvent(StatisticsBulletHitEvent{.author = author});
				break;
			case StatisticsType::TankHit:
				_events->EmitEvent(StatisticsTankHitEvent{.who = who, .author = author});
				break;
			case StatisticsType::TankDied:
				_events->EmitEvent(TankDiedEvent{.who = who, .uuid = cmd.uuid, .author = author});
				break;
			case StatisticsType::BrickWallDied:
				_events->EmitEvent(BrickWallDiedEvent{.author = author});
				break;
			case StatisticsType::SteelWallDied:
				_events->EmitEvent(SteelWallDiedEvent{.author = author});
				break;
			case StatisticsType::BonusPickup:
				_events->EmitEvent(StatisticsBonusPickupEvent{.author = author});
				break;
			case StatisticsType::BonusDestroyed:
				_events->EmitEvent(StatisticsBonusDestroyedEvent{.author = author});
				break;
			case StatisticsType::BonusExpired:
				_events->EmitEvent(StatisticsBonusExpiredEvent{});
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
			_events->EmitEvent(SetPauseEvent{.isPaused = cmd.isPressed});
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
		this->_events->EmitEvent(GameFinishedEvent{.state = cmd.state});
	});
}

void Client::OnBonusSpawn(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<BonusSpawn>(command)]()
	{
		//NOTE: the type indexes the bonus catalogue further down, so a wire byte outside the enum
		//would read past its end - checked here, where the untrusted value enters
		if (!IsSpawnableBonus(cmd.bonusType))
		{
			Log::Error("Client::OnBonusSpawn: bonus type "
					   + std::to_string(static_cast<int>(cmd.bonusType)) + " is not spawnable");

			return;
		}

		_events->EmitEvent(
				BonusSpawnedEvent{.pos = cmd.pos, .type = cmd.bonusType, .uuid = cmd.uuid, .isSuper = cmd.isSuper});
	});
}

void Client::OnRespawnTank(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<RespawnTank>(command)]()
	{
		_events->EmitEvent(TankRespawnedEvent{.type = cmd.tankType, .uuid = cmd.uuid, .pos = cmd.pos});
	});
}

void Client::OnObstacleSpawn(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<ObstacleSpawn>(command)]()
	{
		_events->EmitEvent(ObstacleSpawnedEvent{.pos = cmd.pos, .type = cmd.obstacleType, .uuid = cmd.uuid});
	});
}

void Client::OnTankSpawnComplete(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<TankSpawnComplete>(command)]()
	{
		_events->EmitEvent(TankSpawnCompletedEvent{.uuid = cmd.uuid});
	});
}

void Client::OnBonusSpawnComplete(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<BonusSpawnComplete>(command)]()
	{
		_events->EmitEvent(BonusSpawnCompletedEvent{.uuid = cmd.uuid});
	});
}

void Client::OnTierChange(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<TierChange>(command)]()
	{
		_events->EmitEvent(Key(cmd.uuid), TierChangedEvent{.who = cmd.who, .tier = cmd.tier, .uuid = cmd.uuid});
	});
}

void Client::OnBonusStatus(const AnyCommand& command)
{
	_commandQueue.Enqueue([this, cmd = std::get<BonusStatus>(command)]()
	{
		//NOTE: only the bonuses whose effect the client cannot see any other way are replicated here -
		//a star and a caliber land as a TierChange, the rest are applied once on the host and never
		//reported, so any of them is as wrong here as a byte outside the enum
		switch (cmd.bonusType)
		{
			case BonusType::Helmet:
				_events->EmitEvent(Key(cmd.name), BonusHelmetAppliedEvent{.name = cmd.name, .isActive = cmd.isEnable});
				return;
			case BonusType::Ship:
				_events->EmitEvent(Key(cmd.name), BonusShipAppliedEvent{.name = cmd.name});
				return;
			case BonusType::Tank:
				_events->EmitEvent(BonusTankAppliedEvent{.author = SeatFromWire(cmd.author)});
				return;
			case BonusType::Star:
			case BonusType::Caliber:
			case BonusType::None:
			case BonusType::Timer:
			case BonusType::Grenade:
			case BonusType::Shovel:
			case BonusType::lastId:
				break;
		}

		Log::Error("Client::OnBonusStatus: bonus type " + std::to_string(static_cast<int>(cmd.bonusType))
				   + " is not replicated");
	});
}

void Client::OnDisconnect(const AnyCommand& command)
{
	const auto reason = std::get<Disconnect>(command).reason;

	//NOTE: on the network thread, not in the queued lambda - the EOF arrives well before the game
	//thread drains the queue, and HandleDisconnect must already know why
	_isLinkUnrecoverable = reason == DisconnectReason::ProtocolError;

	_commandQueue.Enqueue([this, reason]()
	{
		_events->EmitEvent(ClientInDisconnectEvent{.reason = reason});
	});
}

}//namespace network::commands
