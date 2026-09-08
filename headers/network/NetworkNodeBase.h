#pragma once

#include "components/EventSystem.h"
#include "interfaces/INetworkNode.h"
#include <functional>
#include <memory>
#include <string>
#include <thread>
#include <vector>

struct NetCommandUpdateEvent;

//NOTE: declared, not included - <boost/asio/io_context.hpp> costs ~239k lines after preprocessing,
//and nothing above the network layer ever needs to see the type
namespace boost::asio
{
class io_context;
}

namespace network::commands
{
//NOTE: the io_context + its thread, shared by ClientNode and ServerNode. Only one thread runs
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

	[[nodiscard]] boost::asio::io_context& IoContext();
	[[nodiscard]] const std::shared_ptr<EventSystem>& Events() const { return _events; }

private:
	void OnNetCommandUpdate(const NetCommandUpdateEvent&);

	std::shared_ptr<EventSystem> _events;
	std::string _name;
	//NOTE: by pointer only so the header can keep asio out; the node owns it either way
	std::unique_ptr<boost::asio::io_context> _ioContext;
	std::thread _thread{};
	std::vector<EventSubscription> _subs{};

	static constexpr int kShutdownTimeoutMs{500};
};
}//namespace network::commands
