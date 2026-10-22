#pragma once

#include "components/EventSystem.h"
#include "enums/PlayerSlot.h"
#include <vector>

class EventSystem;

namespace network
{
class ReplicationPublisher;

//NOTE: what a host announces; apart from the client half, so BattleCityServer reuses it as is
void BindHostReplication(ReplicationPublisher& out);

void BindClientReplication(ReplicationPublisher& out);

//NOTE: called once the server names the seat and again on a new seat; rebinding drops the result and calls again
[[nodiscard]] std::vector<EventSubscription> BindClientInput(ReplicationPublisher& out, EventSystem& events,
															 PlayerSlot slot);
}//namespace network
