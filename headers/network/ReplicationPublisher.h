#pragma once

#include "commands/CommandBatch.h"
#include "components/EventSystem.h"
#include <memory>
#include <mutex>
#include <source_location>
#include <string>
#include <utility>
#include <vector>

class EventSystem;

namespace network::commands
{
class ReplicationPublisher final
{
public:
	explicit ReplicationPublisher(const std::shared_ptr<EventSystem>& events);

	//NOTE: everything published since the last call as one frame, or nullptr when there was nothing
	[[nodiscard]] std::shared_ptr<const std::string> TakeFrame();

	void Publish(AnyCommand command);

	//NOTE: origin defaults here and is forwarded, so the debug listener registry names each Bind call site
	template<class EventT, class ToCommand>
	void Bind(ToCommand toCommand, const std::source_location& origin = std::source_location::current())
	{
		_subs.push_back(_events->AddListener([this, toCommand](const EventT& event)
		{
			Publish(toCommand(event));
		}, origin));
	}

private:
	std::shared_ptr<EventSystem> _events{nullptr};
	std::vector<EventSubscription> _subs{};

	std::mutex _batchWriteMutex;
	CommandBatch _batch{};
};
}//namespace network::commands
