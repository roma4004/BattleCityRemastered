#pragma once

#include "NetworkCommandQueue.h"
#include "commands/AnyCommand.h"
#include <memory>
#include <vector>

struct BonusEffectTimeLeft;
class EventSystem;

namespace network
{
//NOTE: the mirror of ReplicationPublisher: it packs a local event into a command, this unpacks it back again
class ReplicationApplier final
{
public:
	ReplicationApplier(const std::shared_ptr<EventSystem>& events, network::NetworkCommandQueue& queue);

	//NOTE: called on the network thread; hops to the game thread once per command, so no Emit below queues itself
	void Apply(const commands::AnyCommand& command);

private:
	//NOTE: an overload per AnyCommand alternative, no catch-all - a new command fails to compile until handled here
	void Emit(const commands::PositionChange& command) const;
	void Emit(const commands::TankShot& command) const;
	void Emit(const commands::HealthChange& command) const;
	void Emit(const commands::TierChange& command) const;
	void Emit(const commands::TankGrew& command) const;
	void Emit(const commands::Despawn& command) const;
	void Emit(const commands::RespawnTank& command) const;
	void Emit(const commands::RespawnCountChange& command) const;
	void Emit(const commands::ObstacleSpawn& command) const;
	void Emit(const commands::TankSpawnComplete& command) const;
	void Emit(const commands::TankSpawnMoved& command) const;
	void Emit(const commands::BonusSpawnComplete& command) const;
	void Emit(const commands::GameStateChange& command) const;
	void Emit(const commands::KeyStateChange& command) const;
	void Emit(const commands::StatisticsChange& command) const;
	void Emit(const commands::BonusSpawn& command) const;
	void Emit(const commands::BonusStatus& command) const;
	void Emit(const commands::BonusEffects& command) const;
	void Emit(const commands::SlotAssignment& command) const;
	void Emit(const commands::WorldSnapshot& command) const;
	void Emit(const commands::AbsenceChange& command) const;
	void Emit(const commands::PortForwardingChange& command) const;
	void Emit(const commands::SeatHoldersChange& command) const;

	void EmitEffects(std::vector<BonusEffectTimeLeft> effects) const;

	//NOTE: host-bound commands - both peers share AnyCommand, so these reach a client only if it talks to itself
	void Emit(const commands::SignalEvent&) const {}

	//NOTE: transport state, not a game fact - Client handles it on the network thread before the queue
	void Emit(const commands::Disconnect&) const {}

	std::shared_ptr<EventSystem> _events{nullptr};
	network::NetworkCommandQueue& _queue;
};
}//namespace network
