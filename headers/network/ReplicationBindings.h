#pragma once

namespace network::commands
{
class ReplicationPublisher;

//NOTE: the mechanism is one class, the two profiles are these - what a host announces to its
//clients, and the four things a client sends back. Splitting them is what lets one publisher serve
//both ends, and a dedicated server reuse the host profile as it stands.
void BindHostReplication(ReplicationPublisher& out);

void BindClientReplication(ReplicationPublisher& out);
}//namespace network::commands
