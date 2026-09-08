#pragma once

#include "components/EventSystem.h"
#include "enums/PlayerSlot.h"
#include <vector>

class EventSystem;

namespace network::commands
{
class ReplicationPublisher;

//NOTE: one publisher, two profiles - what a host announces and what a client sends back. The split
//is what lets BattleCityServer reuse the host profile as it stands
void BindHostReplication(ReplicationPublisher& out);

void BindClientReplication(ReplicationPublisher& out);

//NOTE: called once the server names the seat, and again if a later connection names another one.
//The caller holds the subscriptions, so rebinding is dropping them and calling this again
[[nodiscard]] std::vector<EventSubscription> BindClientInput(ReplicationPublisher& out, EventSystem& events,
															 PlayerSlot slot);
}//namespace network::commands
