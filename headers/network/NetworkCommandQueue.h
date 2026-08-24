#pragma once

#include <functional>
#include <mutex>
#include <queue>

namespace network
{
class NetworkCommandQueue final
{
	std::queue<std::function<void()>> _commands;
	mutable std::mutex _mutex;

public:
	NetworkCommandQueue() = default;
	~NetworkCommandQueue() = default;

	// Non-copyable
	NetworkCommandQueue(const NetworkCommandQueue&) = delete;
	NetworkCommandQueue& operator=(const NetworkCommandQueue&) = delete;

	void Enqueue(std::function<void()> command)
	{
		std::scoped_lock lock(_mutex);
		_commands.push(std::move(command));
	}

	void ProcessAll()
	{
		std::queue<std::function<void()>> localQueue;
		{
			std::scoped_lock lock(_mutex);
			std::swap(localQueue, _commands);
		}

		while (!localQueue.empty())
		{
			localQueue.front()();
			localQueue.pop();
		}
	}

	[[nodiscard]] size_t Size() const
	{
		std::scoped_lock lock(_mutex);
		return _commands.size();
	}
};
}//namespace network
