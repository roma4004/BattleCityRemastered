#pragma once

#include "NetworkCommandQueue.h"
#include "commands/AnyCommand.h"
#include <memory>

class EventSystem;

namespace network::commands
{
//NOTE: the mirror of ReplicationPublisher - that one packs a local event into a command, this one
//unpacks the command back into the same local event
class ReplicationApplier final
{
public:
	ReplicationApplier(const std::shared_ptr<EventSystem>& events, network::NetworkCommandQueue& queue);

	//NOTE: called on the network thread, and the hop to the game one happens here, once for the whole
	//command - so no Emit below has to remember to queue anything
	void Apply(const AnyCommand& command);

private:
	//NOTE: every alternative of AnyCommand is named on purpose - without a catch-all template, a new
	//command stops compiling until someone decides what the client does with it
	void Emit(const PositionChange& command) const;
	void Emit(const TankShot& command) const;
	void Emit(const HealthChange& command) const;
	void Emit(const TierChange& command) const;
	void Emit(const Despawn& command) const;
	void Emit(const RespawnTank& command) const;
	void Emit(const ObstacleSpawn& command) const;
	void Emit(const TankSpawnComplete& command) const;
	void Emit(const BonusSpawnComplete& command) const;
	void Emit(const GameStateChange& command) const;
	void Emit(const KeyStateChange& command) const;
	void Emit(const StatisticsChange& command) const;
	void Emit(const BonusSpawn& command) const;
	void Emit(const BonusStatus& command) const;

	//NOTE: the host-bound half of the wire - both peers share one AnyCommand, so these reach a client
	//only if it is talking to itself
	void Emit(const SignalEvent&) const {}

	//NOTE: not a game fact but transport state - Client reads it on the network thread, ahead of the
	//queue, and never lets it reach here
	void Emit(const Disconnect&) const {}

	std::shared_ptr<EventSystem> _events{nullptr};
	network::NetworkCommandQueue& _queue;
};
}//namespace network::commands
