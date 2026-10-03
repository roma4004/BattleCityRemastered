#pragma once

#include "Endpoints.h"
#include "NetworkNodeBase.h"
#include "enums/MatchRules.h"
#include "enums/PlayerSlot.h"
#include <cstddef>
#include <cstdint>
#include <memory>

namespace network::commands
{
class Server;

class ServerNode final : public NetworkNodeBase
{
public:
	ServerNode(const ServerAddress& address, const std::shared_ptr<EventSystem>& events,
			   std::size_t seatCount = kDefaultSeats, MatchRules rules = MatchRules::Classic);

	~ServerNode() override;

	void ProcessNetworkCommands() override;

	[[nodiscard]] uint16_t GetBoundPort() const noexcept;

	//NOTE: no goodbye, the way a crashed host would go - the peer hears nothing more, times out and reconnects,
	//unlike the destructor's announced leave
	void Abort();

private:
	//NOTE: by pointer only to keep Server - and with it asio - out of this header
	std::unique_ptr<Server> _server;
};

}//namespace network::commands
