#pragma once

#include "DatagramLink.h"
#include "PeerLink.h"
#include "WireFrame.h"
#include "components/MatchSettings.h"
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

namespace network
{
using boost::asio::ip::udp;

//NOTE: one client, known by address; Server does its I/O, and every method below the accessors runs on the io thread
class Session final : public PeerLink, public std::enable_shared_from_this<Session>
{
public:
	Session(udp::endpoint endpoint, std::uint32_t connectionId, const std::shared_ptr<EventSystem>& events,
			PlayerSlot slot, MatchSettings match, DatagramLink::Clock::time_point now);

	//NOTE: fixed when the server seated the client - a press off the wire names the key, the seat says whose
	[[nodiscard]] PlayerSlot GetSlot() const noexcept { return _slot; }
	[[nodiscard]] std::string Address() const
	{
		return _endpoint.address().to_string() + ':' + std::to_string(_endpoint.port());
	}
	[[nodiscard]] std::chrono::steady_clock::time_point ConnectedAt() const noexcept { return _connectedAt; }

	//NOTE: the cue to drop this session - the game thread reads it without a lock, as it does IsWoundDown
	[[nodiscard]] bool IsFinished() const noexcept { return _isFinished.load(std::memory_order_acquire); }
	//NOTE: finished, and nothing of it left in flight - the goodbyes both ways read, or the peer gone silent
	[[nodiscard]] bool IsWoundDown() const noexcept { return _isWoundDown.load(std::memory_order_acquire); }
	//NOTE: on the network thread, the link's only one - the game thread is handed the answer, not the link
	void UpdateWoundDown(DatagramLink::Clock::time_point now);

	//NOTE: set on ready and after a dropped backlog - the next frame is replaced by a snapshot
	[[nodiscard]] bool IsSnapshotOwed() const { return _isSnapshotOwed.load(std::memory_order_acquire); }
	void OweSnapshot() { _isSnapshotOwed.store(true, std::memory_order_release); }
	void ClearSnapshotDebt() { _isSnapshotOwed.store(false, std::memory_order_release); }

	[[nodiscard]] const udp::endpoint& Endpoint() const { return _endpoint; }
	[[nodiscard]] std::uint32_t ConnectionId() const { return _link.ConnectionId(); }
	[[nodiscard]] bool IsDrained() const { return _link.IsDrained(); }

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
	//NOTE: visited on the network thread; each Handle queues its game-thread work, the goodbye also acts right here
	void OnCommand(const commands::AnyCommand& command) override;

	//NOTE: an overload per AnyCommand alternative, no catch-all - a new command fails to compile until handled here
	void Handle(const commands::SignalEvent& command);
	void Handle(const commands::KeyStateChange& command);
	void Handle(const commands::Disconnect& command);

	//NOTE: the client-bound half - a host writes these, it never reads them
	void Handle(const commands::PositionChange&) const {}
	void Handle(const commands::TankShot&) const {}
	void Handle(const commands::HealthChange&) const {}
	void Handle(const commands::TierChange&) const {}
	void Handle(const commands::TankGrew&) const {}
	void Handle(const commands::Despawn&) const {}
	void Handle(const commands::RespawnTank&) const {}
	void Handle(const commands::ObstacleSpawn&) const {}
	void Handle(const commands::TankSpawnComplete&) const {}
	void Handle(const commands::TankSpawnMoved&) const {}
	void Handle(const commands::BonusSpawnComplete&) const {}
	void Handle(const commands::GameStateChange&) const {}
	void Handle(const commands::StatisticsChange&) const {}
	void Handle(const commands::BonusSpawn&) const {}
	void Handle(const commands::BonusStatus&) const {}
	void Handle(const commands::BonusEffects&) const {}
	void Handle(const commands::SlotAssignment&) const {}
	void Handle(const commands::WorldSnapshot&) const {}
	void Handle(const commands::AbsenceChange&) const {}
	void Handle(const commands::PortForwardingChange&) const {}
	void Handle(const commands::SeatHoldersChange&) const {}

	void Lose();

	//NOTE: a key held when the link went stays held on the host - the seat's tank would drive on alone
	void ReleaseSeatKeys() const;

	//NOTE: raised after the last Enqueue, so everything this session read is queued before cleanup sweeps it
	void MarkFinished() { _isFinished.store(true, std::memory_order_release); }

	const udp::endpoint _endpoint;
	DatagramLink _link;
	const PlayerSlot _slot;
	//NOTE: sent to the client with its seat
	const MatchSettings _match;
	const std::chrono::steady_clock::time_point _connectedAt{std::chrono::steady_clock::now()};
	bool _isPeerGone{};
	std::atomic_bool _isFinished{};
	std::atomic_bool _isWoundDown{};
	std::atomic_bool _isSnapshotOwed{};
};
}//namespace network
