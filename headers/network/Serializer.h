#pragma once

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
[[nodiscard]] std::string SerializeFrame(const commands::CommandBatch& batch);

[[nodiscard]] std::expected<commands::CommandBatch, DeserializeError> Deserialize(const std::string& archiveData);
}// namespace network
