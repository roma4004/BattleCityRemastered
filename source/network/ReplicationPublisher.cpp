#include "network/ReplicationPublisher.h"
#include "network/MessageFraming.h"
#include "network/Serializer.h"
#include <memory>
#include <mutex>
#include <string>
#include <utility>

namespace network::commands
{
ReplicationPublisher::ReplicationPublisher(const std::shared_ptr<EventSystem>& events)
	: _events{events} {}

void ReplicationPublisher::Publish(const AnyCommand command)
{
	const std::scoped_lock lock{_batchWriteMutex};
	_batch.commands.emplace_back(command);
}

std::shared_ptr<const std::string> ReplicationPublisher::TakeFrame()
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

	return std::make_shared<const std::string>(network::SerializeFrame(batch));
}

}//namespace network::commands
