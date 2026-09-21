#include "network/ReplicationPublisher.h"
#include "network/Serializer.h"
#include "network/WireFrame.h"
#include "utils/Uuid.h"
#include <algorithm>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <variant>

namespace network::commands
{
ReplicationPublisher::ReplicationPublisher(const std::shared_ptr<EventSystem>& events)
	: _events{events} {}

void ReplicationPublisher::Publish(const AnyCommand command)
{
	const std::scoped_lock lock{_batchWriteMutex};
	_batch.commands.emplace_back(command);
}

std::shared_ptr<const WireFrame> ReplicationPublisher::TakeFrame()
{
	CommandBatch batch;
	{
		const std::scoped_lock lock{_batchWriteMutex};
		std::swap(batch, _batch);
	}

	if (batch.commands.empty())
	{
		return nullptr;
	}

	const auto frame{std::make_shared<WireFrame>()};
	CommandBatch reliable;
	std::ranges::for_each(batch.commands, [&frame, &reliable](AnyCommand& command)
	{
		const auto latestKey{std::visit([]<class CommandT>(const CommandT& alternative) -> std::optional<Uuid>
		{
			if constexpr (LatestCommand<CommandT>)
			{
				return alternative.uuid;
			}
			else
			{
				return std::nullopt;
			}
		}, command)};

		if (!latestKey)
		{
			reliable.commands.push_back(std::move(command));
			return;
		}

		CommandBatch single;
		single.commands.push_back(std::move(command));
		frame->latest.emplace_back(*latestKey, network::Serialize(single));
	});

	if (!reliable.commands.empty())
	{
		frame->reliable = network::Serialize(reliable);
	}

	return frame;
}

}//namespace network::commands
