#pragma once

#include "Endpoints.h"
#include "NetworkNodeBase.h"
#include "components/MatchSettings.h"
#include <cstdint>
#include <memory>
#include <optional>

namespace network
{
class Server;

class ServerNode final : public NetworkNodeBase
{
public:
	ServerNode(const ServerAddress& address, const std::shared_ptr<EventSystem>& events,
			   const MatchSettings& match = {}, const std::optional<ServerAddress>& catalog = std::nullopt);

	~ServerNode() override;

	void ProcessNetworkCommands() override;

	[[nodiscard]] uint16_t GetBoundPort() const noexcept;

private:
	std::unique_ptr<Server> _server;
};

}//namespace network
