#pragma once

#include "components/EventSystem.h"
#include "interfaces/INetworkNode.h"
#include <boost/asio/io_context.hpp>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

struct NetCommandUpdateEvent;

namespace network::commands
{
//NOTE: the io_context + its thread, shared by ClientHandler and ServerHandler. Only one thread runs
//it, which is what makes the strands inside Client/Session enough on their own.
class NetworkNodeBase : public INetworkNode
{
public:
	~NetworkNodeBase() override;

protected:
	NetworkNodeBase(std::shared_ptr<EventSystem> events, std::string name);

	void StartIoThread();

	//NOTE: call from the derived destructor, while the node it shuts down is still alive. `done`
	//instead of a plain return: the goodbye is a frame write, so it finishes asynchronously.
	void StopIoThread(const std::function<void(std::function<void()>)>& shutdownNode);

	//NOTE: not in the constructor - it would expose a half-built object through ProcessNetworkCommands
	void SubscribeToNetCommandUpdate();

	[[nodiscard]] boost::asio::io_context& IoContext() { return _ioContext; }
	[[nodiscard]] const std::shared_ptr<EventSystem>& Events() const { return _events; }

private:
	void OnNetCommandUpdate(const NetCommandUpdateEvent&);

	std::shared_ptr<EventSystem> _events;
	std::string _name;
	boost::asio::io_context _ioContext{};
	std::thread _thread{};
	std::vector<EventSubscription> _subs{};

	static constexpr int ShutdownTimeoutMs{500};
};
}//namespace network::commands
