#pragma once

#include "enums/DisconnectReason.h"
#include <expected>
#include <string>

namespace network::commands
{
struct CommandBatch;
}

namespace network
{
struct DeserializeError final
{
	std::string reason{};
};

//NOTE: the only place that builds a ser20 archive, so the wire format has one owner
[[nodiscard]] std::string Serialize(const commands::CommandBatch& batch);

//NOTE: a goodbye is a Disconnect alone, whoever says it
[[nodiscard]] std::string SerializeGoodbye(DisconnectReason reason);

[[nodiscard]] std::expected<commands::CommandBatch, DeserializeError> Deserialize(const std::string& archiveData);
}// namespace network
