#include "network/NetworkNodeBase.h"
#include "components/events/TimingEvents.h"
#include "utils/NetworkLogger.h"
#include <boost/asio/post.hpp>
#include <chrono>
#include <future>
#include <utility>

namespace network::commands
{
NetworkNodeBase::NetworkNodeBase(std::shared_ptr<EventSystem> events, std::string name)
	: _events(std::move(events))
	, _name(std::move(name))
{
}

NetworkNodeBase::~NetworkNodeBase() = default;

void NetworkNodeBase::StartIoThread()
{
	_thread = std::thread([this]
	{
		try
		{
			_ioContext.run();
		}
		catch (const std::exception& e)
		{
			NetworkLogger::WriteError(_name + " thread exception: " + e.what());
		}
		catch (...)
		{
			NetworkLogger::WriteError(_name + " thread error: unknown exception");
		}
	});
}

void NetworkNodeBase::StopIoThread(const std::function<void(std::function<void()>)>& shutdownNode)
{
	//NOTE: the node is shut down on its own io_context thread, not here - closing sockets from
	//outside would race the handlers still queued on it
	if (shutdownNode && !_ioContext.stopped())
	{
		auto shutdownDone{std::make_shared<std::promise<void>>()};
		const std::future<void> shutdownFuture{shutdownDone->get_future()};

		boost::asio::post(_ioContext, [shutdownNode, shutdownDone]
		{
			//NOTE: guarded - a node with several links reports each one, and set_value twice throws
			shutdownNode([shutdownDone, isDone = std::make_shared<bool>(false)]
			{
				if (!*isDone)
				{
					*isDone = true;
					shutdownDone->set_value();
				}
			});
		});

		shutdownFuture.wait_for(std::chrono::milliseconds(ShutdownTimeoutMs));
	}

	if (!_ioContext.stopped())
	{
		_ioContext.stop();
	}

	if (_thread.joinable())
	{
		_thread.join();
	}
}

void NetworkNodeBase::SubscribeToNetCommandUpdate()
{
	_subs.push_back(_events->AddListener(this, &NetworkNodeBase::OnNetCommandUpdate));
}

void NetworkNodeBase::OnNetCommandUpdate(const NetCommandUpdateEvent&) { ProcessNetworkCommands(); }
}//namespace network::commands
