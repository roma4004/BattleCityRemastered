#pragma once

#include "FrameChannel.h"
#include "NetworkCommandQueue.h"
#include "commands/AnyCommand.h"
#include "commands/CommandBatch.h"
#include "enums/DisconnectReason.h"
#include <boost/asio/ip/tcp.hpp>
#include <functional>
#include <memory>
#include <string>

class EventSystem;

namespace network::commands
{
using boost::asio::ip::tcp;

class PeerLink
{
public:
	PeerLink(const PeerLink&) = delete;
	PeerLink& operator=(const PeerLink&) = delete;
	PeerLink(PeerLink&&) = delete;
	PeerLink& operator=(PeerLink&&) = delete;

	void ProcessCommandQueue() { _commandQueue.ProcessAll(); }

	[[nodiscard]] bool IsSocketOpen() const { return _channel->IsOpen(); }
	[[nodiscard]] bool HasPendingCommands() const { return _commandQueue.Size() > 0u; }

protected:
	~PeerLink() = default;

	PeerLink(tcp::socket socket, std::string ownerName, std::shared_ptr<EventSystem> events);

	//NOTE: what this peer makes of a command - Client takes the goodbye off the network thread first,
	//Session visits straight away. Called from DispatchFrame, so never before the peer is built.
	virtual void OnCommand(const AnyCommand& command) = 0;

	//NOTE: false only for a frame that will not parse. A command the peer ignores is not a failure -
	//both ends share one AnyCommand, and each drops the half addressed to the other.
	[[nodiscard]] bool DispatchFrame(const std::string& frame);

	void SendBatch(const CommandBatch& batch);

	void CloseWithFarewell(bool hasLink, DisconnectReason reason, std::function<void()> onClosed);

	std::shared_ptr<network::FrameChannel> _channel;
	std::shared_ptr<EventSystem> _events{nullptr};
	network::NetworkCommandQueue _commandQueue;
	std::string _ownerName;
};
}//namespace network::commands
