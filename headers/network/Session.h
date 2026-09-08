#pragma once

#include "PeerLink.h"
#include "enums/DisconnectReason.h"
#include "enums/InputSignal.h"
#include "enums/PlayerSlot.h"
#include <atomic>
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
	Session(tcp::socket sock, const std::shared_ptr<EventSystem>& events, PlayerSlot slot);

	~Session();

	//NOTE: the seat is the session's, fixed when the server accepted it - a press off the wire names
	//the key, and the seat says whose it is
	[[nodiscard]] PlayerSlot GetSlot() const { return _slot; }

	//NOTE: the cue to drop this session, and the only field the game thread reads without a lock
	[[nodiscard]] bool IsFinished() const { return _isFinished.load(std::memory_order_acquire); }

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
	void Handle(const SlotAssignment&) const {}

	//NOTE: raised on our strand after the last Enqueue, so everything this session read is in the
	//queue by the time cleanup is free to sweep it
	void MarkFinished() { _isFinished.store(true, std::memory_order_release); }

	static const std::unordered_map<InputSignal, InputEmitter> kInputEmitters;

	const PlayerSlot _slot;
	bool _isPeerGone{false};
	std::atomic_bool _isFinished{false};
};
}//namespace network::commands
