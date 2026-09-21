#pragma once

#include "DatagramLink.h"
#include "NetworkCommandQueue.h"
#include "commands/AnyCommand.h"
#include <memory>
#include <string>

class EventSystem;

namespace network::commands
{
class PeerLink
{
public:
	PeerLink(const PeerLink&) = delete;
	PeerLink& operator=(const PeerLink&) = delete;
	PeerLink(PeerLink&&) = delete;
	PeerLink& operator=(PeerLink&&) = delete;

	void ProcessCommandQueue() { _commandQueue.ProcessAll(); }

	[[nodiscard]] bool HasPendingCommands() const { return _commandQueue.Size() > 0u; }

protected:
	~PeerLink() = default;

	PeerLink(std::string ownerName, std::shared_ptr<EventSystem> events);

	//NOTE: what this peer makes of a command - Client takes the goodbye off the network thread first,
	//Session visits straight away. Called from Dispatch, so never before the peer is built.
	virtual void OnCommand(const AnyCommand& command) = 0;

	//NOTE: false only for a message that will not parse. A command the peer ignores is not a failure -
	//both ends share one AnyCommand, and each drops the half addressed to the other.
	[[nodiscard]] bool Dispatch(const DatagramLink::Arrivals& arrivals);

	std::shared_ptr<EventSystem> _events{nullptr};
	network::NetworkCommandQueue _commandQueue;
	std::string _ownerName;

private:
	[[nodiscard]] bool DispatchMessage(const std::string& message);
};
}//namespace network::commands
