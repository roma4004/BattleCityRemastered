#pragma once

#include "CommandDispatcher.h"
#include "FrameChannel.h"
#include "NetworkCommandQueue.h"
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

	[[nodiscard]] bool DispatchFrame(const std::string& frame) { return _dispatcher.Dispatch(frame).has_value(); }

	void SendBatch(const CommandBatch& batch);

	void CloseWithFarewell(bool hasLink, DisconnectReason reason, std::function<void()> onClosed);

	std::shared_ptr<network::FrameChannel> _channel;
	std::shared_ptr<EventSystem> _events{nullptr};
	network::NetworkCommandQueue _commandQueue;
	network::CommandDispatcher _dispatcher;
};
}//namespace network::commands
