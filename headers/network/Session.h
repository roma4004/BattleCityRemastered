#pragma once

#include "PeerLink.h"
#include "enums/DisconnectReason.h"
#include "enums/InputSignal.h"
#include "enums/PlayerSlot.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>

class EventSystem;

namespace network::commands
{
using boost::asio::ip::tcp;

//NOTE: the incoming half of the server - one per connected client. Outgoing replication is Server's.
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
	using InputEmitter = std::function<void(EventSystem&, PlayerSlot, bool)>;

	//NOTE: visited straight on the network thread, and each Handle queues its own game-thread work -
	//the goodbye has a half that must run right here, ahead of the queue
	void OnCommand(const AnyCommand& command) override;

	//NOTE: every alternative of AnyCommand is named on purpose - without a catch-all template, a new
	//command stops compiling until someone decides whether the host half of the wire cares about it
	void Handle(const SignalEvent& command);
	void Handle(const KeyStateChange& command);
	void Handle(const Disconnect& command);

	//NOTE: the client-bound half - a host writes these, it never reads them
	void Handle(const PositionChange&) const {}
	void Handle(const TankShot&) const {}
	void Handle(const HealthChange&) const {}
	void Handle(const TierChange&) const {}
	void Handle(const Despawn&) const {}
	void Handle(const RespawnTank&) const {}
	void Handle(const ObstacleSpawn&) const {}
	void Handle(const TankSpawnComplete&) const {}
	void Handle(const BonusSpawnComplete&) const {}
	void Handle(const GameStateChange&) const {}
	void Handle(const StatisticsChange&) const {}
	void Handle(const BonusSpawn&) const {}
	void Handle(const BonusStatus&) const {}

	static const std::unordered_map<InputSignal, InputEmitter> kInputEmitters;

	bool _isPeerGone{false};
};
}//namespace network::commands
