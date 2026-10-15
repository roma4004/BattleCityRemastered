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

	//NOTE: called on the network thread, and the hop to the game one happens here, once for the whole
	//command - so no Emit below has to remember to queue anything
	void Apply(const commands::AnyCommand& command);

private:
	//NOTE: every alternative of AnyCommand is named on purpose - without a catch-all template, a new
	//command stops compiling until someone decides what the client does with it
	void Emit(const commands::PositionChange& command) const;
	void Emit(const commands::TankShot& command) const;
	void Emit(const commands::HealthChange& command) const;
	void Emit(const commands::TierChange& command) const;
	void Emit(const commands::TankGrew& command) const;
	void Emit(const commands::Despawn& command) const;
	void Emit(const commands::RespawnTank& command) const;
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

	//NOTE: the host-bound half of the wire - both peers share one AnyCommand, so these reach a client
	//only if it is talking to itself
	void Emit(const commands::SignalEvent&) const {}

	//NOTE: not a game fact but transport state - Client reads it on the network thread, ahead of the
	//queue, and never lets it reach here
	void Emit(const commands::Disconnect&) const {}

	std::shared_ptr<EventSystem> _events{nullptr};
	network::NetworkCommandQueue& _queue;
};
}//namespace network
