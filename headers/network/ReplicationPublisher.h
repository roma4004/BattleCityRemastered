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

	//NOTE: one frame out, or nothing when there was nothing to say. The archive and the length prefix
	//are the same on every node; delivery is not - one socket for a client, every session for a server
	[[nodiscard]] std::shared_ptr<const std::string> TakeFrame();

	//NOTE: for what no local event announces - the readiness a client sends the moment its socket
	//connects, from the io thread
	void Publish(AnyCommand command);

	//NOTE: origin is forwarded, not defaulted inside - otherwise every Bind call site collapses onto
	//this one line in the debug listener registry
	template<class EventT, class ToCommand>
	void Bind(ToCommand toCommand, const std::source_location& origin = std::source_location::current())
	{
		_subs.push_back(_events->AddListener([this, toCommand](const EventT& event)
		{
			Publish(toCommand(event));
		}, origin));
	}

	//NOTE: the keyed half of the same thing - a client's keyboard belongs to a seat, so its input
	//listeners sit under that channel instead of the broadcast bucket
	template<class EventT, class KeyT, class ToCommand>
	void Bind(detail::EventKey<KeyT> key, ToCommand toCommand,
			  const std::source_location& origin = std::source_location::current())
	{
		_subs.push_back(_events->AddListener(key, [this, toCommand](const EventT& event)
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
