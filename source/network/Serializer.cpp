#include "network/Serializer.h"
#include "network/commands/CommandSerialization.h"
#include "network/MessageFraming.h"
#include "utils/Log.h"
#include <ser20/archives/portable_binary.hpp>
#include <array>
#include <cstdint>
#include <sstream>
#include <string>
#include <variant>

namespace network
{
namespace
{
using namespace network::commands;

//NOTE: the name sits beside the type it names, so reordering the variant cannot make the log lie -
//and an alternative added without a name here does not compile, there is no catch-all overload
constexpr const char* NameOf(const PositionChange&) { return "PositionChange"; }
constexpr const char* NameOf(const TankShot&) { return "TankShot"; }
constexpr const char* NameOf(const HealthChange&) { return "HealthChange"; }
constexpr const char* NameOf(const TierChange&) { return "TierChange"; }
constexpr const char* NameOf(const Despawn&) { return "Despawn"; }
constexpr const char* NameOf(const StatisticsChange&) { return "StatisticsChange"; }
constexpr const char* NameOf(const KeyStateChange&) { return "KeyStateChange"; }
constexpr const char* NameOf(const GameStateChange&) { return "GameStateChange"; }
constexpr const char* NameOf(const BonusSpawn&) { return "BonusSpawn"; }
constexpr const char* NameOf(const BonusStatus&) { return "BonusStatus"; }
constexpr const char* NameOf(const RespawnTank&) { return "RespawnTank"; }
constexpr const char* NameOf(const ObstacleSpawn&) { return "ObstacleSpawn"; }
constexpr const char* NameOf(const TankSpawnComplete&) { return "TankSpawnComplete"; }
constexpr const char* NameOf(const BonusSpawnComplete&) { return "BonusSpawnComplete"; }
constexpr const char* NameOf(const SignalEvent&) { return "SignalEvent"; }
constexpr const char* NameOf(const SlotAssignment&) { return "SlotAssignment"; }
constexpr const char* NameOf(const Disconnect&) { return "Disconnect"; }

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

std::string SerializeFrame(const commands::CommandBatch& batch)
{
	std::ostringstream archiveStream;
	//NOTE: the length goes in last, over these four - so the payload is never copied to gain a prefix
	static constexpr std::array<char, kFrameHeaderSize> placeholder{};
	archiveStream.write(placeholder.data(), kFrameHeaderSize);
	{
		ser20::PortableBinaryOutputArchive oa(archiveStream);
		oa(batch);
	}

	//NOTE: Describe walks the batch, so ask before paying for it
	if (Log::IsDetailed())
	{
		Log::Detail("send: " + Describe(batch));
	}

	std::string frame{archiveStream.str()};
	EncodeFrameHeader(frame.data(), static_cast<std::uint32_t>(frame.size() - kFrameHeaderSize));

	return frame;
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
