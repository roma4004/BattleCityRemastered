#include "network/Serializer.h"
#include "network/commands/CommandSerialization.h"
#include "utils/Log.h"
#include <ser20/archives/portable_binary.hpp>
#include <sstream>
#include <string>
#include <variant>

namespace network
{
namespace
{
//NOTE: the name sits beside the type it names, so reordering the variant cannot make the log lie -
//and an alternative added without a name here does not compile, there is no catch-all overload
constexpr const char* NameOf(const commands::PositionChange&) { return "PositionChange"; }
constexpr const char* NameOf(const commands::TankShot&) { return "TankShot"; }
constexpr const char* NameOf(const commands::HealthChange&) { return "HealthChange"; }
constexpr const char* NameOf(const commands::TierChange&) { return "TierChange"; }
constexpr const char* NameOf(const commands::Despawn&) { return "Despawn"; }
constexpr const char* NameOf(const commands::StatisticsChange&) { return "StatisticsChange"; }
constexpr const char* NameOf(const commands::KeyStateChange&) { return "KeyStateChange"; }
constexpr const char* NameOf(const commands::GameStateChange&) { return "GameStateChange"; }
constexpr const char* NameOf(const commands::BonusSpawn&) { return "BonusSpawn"; }
constexpr const char* NameOf(const commands::BonusStatus&) { return "BonusStatus"; }
constexpr const char* NameOf(const commands::RespawnTank&) { return "RespawnTank"; }
constexpr const char* NameOf(const commands::ObstacleSpawn&) { return "ObstacleSpawn"; }
constexpr const char* NameOf(const commands::TankSpawnComplete&) { return "TankSpawnComplete"; }
constexpr const char* NameOf(const commands::TankSpawnMoved&) { return "TankSpawnMoved"; }
constexpr const char* NameOf(const commands::BonusSpawnComplete&) { return "BonusSpawnComplete"; }
constexpr const char* NameOf(const commands::SignalEvent&) { return "SignalEvent"; }
constexpr const char* NameOf(const commands::SlotAssignment&) { return "SlotAssignment"; }
constexpr const char* NameOf(const commands::Disconnect&) { return "Disconnect"; }
constexpr const char* NameOf(const WorldSnapshot&) { return "WorldSnapshot"; }
constexpr const char* NameOf(const commands::AbsenceChange&) { return "AbsenceChange"; }
constexpr const char* NameOf(const commands::PortForwardingChange&) { return "PortForwardingChange"; }

std::string Describe(const commands::CommandBatch& batch)
{
	std::string names;
	for (const auto& command: batch.commands)
	{
		if (!names.empty())
		{
			names += ", ";
		}
		names += std::visit([](const auto& alternative) { return NameOf(alternative); }, command);
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
