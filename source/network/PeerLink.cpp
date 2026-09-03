#include "network/PeerLink.h"
#include "network/Serializer.h"
#include "network/commands/Disconnect.h"
#include "utils/Log.h"
#include <cstddef>
#include <string>
#include <utility>

namespace network::commands
{
PeerLink::PeerLink(tcp::socket socket, std::string ownerName, std::shared_ptr<EventSystem> events)
	: _channel{std::make_shared<network::FrameChannel>(std::move(socket), ownerName)}
	, _events{std::move(events)}
	, _ownerName{std::move(ownerName)} {}

bool PeerLink::DispatchFrame(const std::string& frame)
{
	const auto batch = network::Deserialize(frame);
	if (!batch)
	{
		constexpr std::size_t maxLoggedBytes{200};
		const std::string rawData =
				frame.length() < maxLoggedBytes ? frame : frame.substr(0, maxLoggedBytes) + "...";

		Log::Error(_ownerName + " deserialization: " + batch.error().reason + ", raw size "
				   + std::to_string(frame.length()) + ", raw data: " + rawData);

		return false;
	}

	for (const auto& command: batch->commands)
	{
		OnCommand(command);
	}

	return true;
}

void PeerLink::SendBatch(const CommandBatch& batch)
{
	_channel->Send(std::make_shared<const std::string>(network::FrameMessage(network::Serialize(batch))));
}

void PeerLink::CloseWithFarewell(const bool hasLink, const DisconnectReason reason, std::function<void()> onClosed)
{
	if (!hasLink)
	{
		_channel->Close();
		if (onClosed)
		{
			onClosed();
		}

		return;
	}

	CommandBatch farewell;
	farewell.commands.emplace_back(Disconnect{.reason = reason});
	//NOTE: straight out, not into the per-frame batch - what flushes that stops running at shutdown
	SendBatch(farewell);

	_channel->CloseAfterFlush(std::move(onClosed));
}
}//namespace network::commands
