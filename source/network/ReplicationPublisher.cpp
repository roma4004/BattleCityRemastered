#include "network/ReplicationPublisher.h"
#include "network/Serializer.h"
#include "network/WireFrame.h"
#include "network/commands/AnyCommand.h"
#include "utils/Uuid.h"
#include <algorithm>
#include <memory>
#include <mutex>
#include <optional>
#include <utility>
#include <variant>

namespace network
{
ReplicationPublisher::ReplicationPublisher(const std::shared_ptr<EventSystem>& events)
	: _events{events} {}

void ReplicationPublisher::Publish(commands::AnyCommand command)
{
	const std::scoped_lock lock{_batchWriteMutex};
	_batch.commands.push_back(std::move(command));
}

std::shared_ptr<const WireFrame> ReplicationPublisher::TakeFrame()
{
	commands::CommandBatch batch;
	{
		const std::scoped_lock lock{_batchWriteMutex};
		std::swap(batch, _batch);
	}

	if (batch.commands.empty())
	{
		return nullptr;
	}

	const auto frame{std::make_shared<WireFrame>()};
	commands::CommandBatch reliable;
	std::ranges::for_each(batch.commands, [&frame, &reliable](commands::AnyCommand& command)
	{
		const auto [entity, isLatest]{std::visit([]<class CommandT>(const CommandT& alternative)
		{
			if constexpr (commands::EntityCommand<CommandT>)
			{
				return std::pair{std::optional<Uuid>{alternative.uuid}, commands::LatestCommand<CommandT>};
			}
			else
			{
				return std::pair{std::optional<Uuid>{}, false};
			}
		}, command)};

		if (!isLatest)
		{
			if (entity)
			{
				frame->reliableEntities.push_back(*entity);
			}

			reliable.commands.push_back(std::move(command));
			return;
		}

		commands::CommandBatch single;
		single.commands.push_back(std::move(command));
		frame->latest.emplace_back(*entity, Serialize(single));
	});

	if (!reliable.commands.empty())
	{
		frame->reliable = Serialize(reliable);
	}

	return frame;
}

}//namespace network
