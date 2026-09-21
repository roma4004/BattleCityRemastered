#include "network/PeerLink.h"
#include "network/DatagramLink.h"
#include "network/Serializer.h"
#include "network/commands/CommandBatch.h"
#include "utils/Log.h"
#include <algorithm>
#include <cstddef>
#include <string>
#include <utility>

namespace network::commands
{
PeerLink::PeerLink(std::string ownerName, std::shared_ptr<EventSystem> events)
	: _events{std::move(events)}
	, _ownerName{std::move(ownerName)} {}

//NOTE: the messages first - a Latest value in the same datagram may belong to an entity one of them spawns
bool PeerLink::Dispatch(const DatagramLink::Arrivals& arrivals)
{
	const auto dispatch = [this](const std::string& message) { return DispatchMessage(message); };

	return std::ranges::all_of(arrivals.messages, dispatch) && std::ranges::all_of(arrivals.latest, dispatch);
}

bool PeerLink::DispatchMessage(const std::string& message)
{
	const auto batch{network::Deserialize(message)};
	if (!batch)
	{
		constexpr std::size_t maxLoggedBytes{200};
		const std::string rawData{
				message.length() < maxLoggedBytes ? message : message.substr(0, maxLoggedBytes) + "..."};

		Log::Error(_ownerName + " deserialization: " + batch.error().reason + ", raw size "
				   + std::to_string(message.length()) + ", raw data: " + rawData);

		return false;
	}

	for (const auto& command: batch->commands)
	{
		OnCommand(command);
	}

	return true;
}
}//namespace network::commands
