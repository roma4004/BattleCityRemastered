#pragma once

#include "components/EventSystem.h"
#include "interfaces/INetworkNode.h"
#include <chrono>
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

struct NetCommandUpdateEvent;

namespace boost::asio
{
class io_context;
}

namespace network
{
//NOTE: the io_context and its one thread for ClientNode and ServerNode - one thread, so handlers need no lock
class NetworkNodeBase : public INetworkNode
{
public:
	~NetworkNodeBase() override;

protected:
	NetworkNodeBase(std::shared_ptr<EventSystem> events, std::string name);

	void StartIoThread();

	//NOTE: call from the derived destructor while the node lives; shutdownNode gets `done`, as the goodbye is async
	void StopIoThread(const std::function<void(std::function<void()>)>& shutdownNode);

	//NOTE: not in the constructor - it would expose a half-built object through ProcessNetworkCommands
	void SubscribeToNetCommandUpdate();

	[[nodiscard]] boost::asio::io_context& IoContext();

private:
	void OnNetCommandUpdate(const NetCommandUpdateEvent&);

	std::shared_ptr<EventSystem> _events;
	std::string _name;
	//NOTE: by pointer only so the header can keep asio out; the node owns it either way
	std::unique_ptr<boost::asio::io_context> _ioContext;
	std::thread _thread{};
	std::vector<EventSubscription> _subs{};

	static constexpr std::chrono::milliseconds kShutdownTimeout{500};
};
}//namespace network
