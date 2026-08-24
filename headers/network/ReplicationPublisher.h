#pragma once

#include "commands/CommandBatch.h"
#include "components/EventSystem.h"
#include <memory>
#include <mutex>
#include <source_location>
#include <vector>

class EventSystem;

namespace network::commands
{
class ReplicationPublisher final
{
public:
	explicit ReplicationPublisher(const std::shared_ptr<EventSystem>& events);

	[[nodiscard]] CommandBatch TakeBatch();

private:
	void Subscribe();
	void SubscribeStatistics();
	void SubscribeBonus();

	//NOTE: origin is forwarded, not defaulted inside - otherwise every Bind call site collapses onto
	//this one line in the debug listener registry
	template<class EventT, class ToCommand>
	void Bind(ToCommand toCommand, const std::source_location& origin = std::source_location::current())
	{
		_subs.push_back(_events->AddListener([this, toCommand](const EventT& event)
		{
			std::scoped_lock lock(_batchWriteMutex);
			_batch.commands.emplace_back(toCommand(event));
		}, origin));
	}

	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	std::mutex _batchWriteMutex;
	CommandBatch _batch{};
};
}//namespace network::commands
