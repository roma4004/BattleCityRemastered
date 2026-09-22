#pragma once

#include "DatagramLink.h"
#include "PeerLink.h"
#include "WireFrame.h"
#include "enums/DisconnectReason.h"
#include "enums/PlayerSlot.h"
#include <atomic>
#include <boost/asio/ip/udp.hpp>
#include <chrono>
#include <cstdint>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

class EventSystem;

namespace network::commands
{
using boost::asio::ip::udp;

//NOTE: one client of the server, known by its address. It owns no socket - Server reads and writes for all
//sessions on its own, on its io thread, and every method below the accessors runs there
class Session final : public PeerLink, public std::enable_shared_from_this<Session>
{
public:
	Session(udp::endpoint endpoint, std::uint32_t connectionId, const std::shared_ptr<EventSystem>& events,
			PlayerSlot slot, DatagramLink::Clock::time_point now);

	//NOTE: the seat is the session's, fixed when the server took the client in - a press off the wire names
	//the key, and the seat says whose it is
	[[nodiscard]] PlayerSlot GetSlot() const noexcept { return _slot; }
	[[nodiscard]] std::string Address() const
	{
		return _endpoint.address().to_string() + ':' + std::to_string(_endpoint.port());
	}
	[[nodiscard]] std::chrono::steady_clock::time_point ConnectedAt() const noexcept { return _connectedAt; }

	//NOTE: the cue to drop this session, and the only field the game thread reads without a lock
	[[nodiscard]] bool IsFinished() const noexcept { return _isFinished.load(std::memory_order_acquire); }

	//NOTE: owed on ready and after a dropped backlog - paid with a snapshot in place of the next frame
	[[nodiscard]] bool IsSnapshotOwed() const { return _isSnapshotOwed.load(std::memory_order_acquire); }
	void ClearSnapshotDebt() { _isSnapshotOwed.store(false, std::memory_order_release); }

	[[nodiscard]] const udp::endpoint& Endpoint() const { return _endpoint; }
	[[nodiscard]] std::uint32_t ConnectionId() const { return _link.ConnectionId(); }
	[[nodiscard]] bool IsDrained() const { return _link.IsDrained(); }
	[[nodiscard]] bool IsSilent(DatagramLink::Clock::time_point now) const { return _link.IsSilent(now); }

	void Start();
	void Receive(std::string_view datagram, DatagramLink::Clock::time_point now);
	void Send(const WireFrame& frame);
	[[nodiscard]] std::vector<std::string> TakeDatagrams(DatagramLink::Clock::time_point now);

	void LoseIfSilent(DatagramLink::Clock::time_point now);
	//NOTE: the same address dialled again under a new connection - the old one is gone, whatever it said
	void Supersede();
	void Shutdown(DisconnectReason reason);
	//NOTE: told why, so the client does not dial straight back into the seat
	void Kick();

private:
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
	void Handle(const WorldSnapshot&) const {}

	void Lose();

	//NOTE: a key held when the link went stays held on the host - the seat's tank would drive on alone
	void ReleaseSeatKeys() const;

	//NOTE: raised after the last Enqueue, so everything this session read is queued before cleanup sweeps it
	void MarkFinished() { _isFinished.store(true, std::memory_order_release); }

	const udp::endpoint _endpoint;
	DatagramLink _link;
	const PlayerSlot _slot;
	const std::chrono::steady_clock::time_point _connectedAt{std::chrono::steady_clock::now()};
	bool _isPeerGone{};
	std::atomic_bool _isFinished{};
	std::atomic_bool _isSnapshotOwed{};
};
}//namespace network::commands
