#pragma once

#include "DatagramLink.h"
#include "NetworkCommandQueue.h"
#include "commands/AnyCommand.h"
#include <memory>
#include <string>

class EventSystem;

namespace network
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

	//NOTE: called only from Dispatch, so never before the peer is built
	virtual void OnCommand(const commands::AnyCommand& command) = 0;

	//NOTE: false only for an unparsable message; both ends share AnyCommand, so ignoring the other half is fine
	[[nodiscard]] bool Dispatch(const DatagramLink::Arrivals& arrivals);

	std::shared_ptr<EventSystem> _events{nullptr};
	network::NetworkCommandQueue _commandQueue;
	std::string _ownerName;

private:
	[[nodiscard]] bool DispatchMessage(const std::string& message);
};
}//namespace network
