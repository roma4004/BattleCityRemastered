#include "network/Serializer.h"
#include "network/commands/CommandSerialization.h"
#include "utils/Log.h"
#include <array>
#include <ser20/archives/portable_binary.hpp>
#include <sstream>

namespace network
{
namespace
{
//NOTE: the last enumerator, so adding a command without a name here fails to compile
constexpr std::size_t kCommandTypeCount{static_cast<std::size_t>(CommandType::DISCONNECT) + 1u};

constexpr std::array<const char*, kCommandTypeCount> kCommandNames{
		"CommandBatch", "PositionChange", "TankShot", "HealthChange", "Despawn", "StatisticsChange",
		"KeyStateChange", "GameStateChange", "BonusSpawn", "BonusStatus",
		"RespawnTank", "ObstacleSpawn", "TankSpawnComplete", "SignalEvent", "Disconnect"};

std::string Describe(const commands::CommandBatch& batch)
{
	std::string names;
	for (const auto& command: batch.commands)
	{
		if (!names.empty())
		{
			names += ", ";
		}
		names += kCommandNames[static_cast<std::size_t>(commands::GetCommandType(command))];
	}

	return names;
}
}// namespace

std::string Serialize(const commands::CommandBatch& batch)
{
	std::ostringstream archiveStream;
	{
		ser20::PortableBinaryOutputArchive oa(archiveStream);
		oa(batch);
	}

	//NOTE: Describe walks the batch, so ask before paying for it
	if (Log::IsDetailed())
	{
		Log::Detail("send: " + Describe(batch));
	}

	return archiveStream.str();
}

std::expected<commands::CommandBatch, DeserializeError> Deserialize(const std::string& archiveData)
{
	commands::CommandBatch batch;
	try
	{
		std::istringstream archiveStream(archiveData);
		ser20::PortableBinaryInputArchive ia(archiveStream);
		ia(batch);
	}
	catch (const std::exception& e)
	{
		return std::unexpected(DeserializeError{.reason = e.what()});
	}

	if (Log::IsDetailed())
	{
		Log::Detail("recv: " + Describe(batch));
	}

	return batch;
}
}// namespace network
