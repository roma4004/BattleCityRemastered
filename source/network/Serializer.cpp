#include "network/Serializer.h"
#include "network/commands/CommandBatch.h"
#include "network/commands/CommandSerialization.h"
#include "network/commands/Disconnect.h"
#include "enums/DisconnectReason.h"
#include "utils/Log.h"
#include <ser20/archives/portable_binary.hpp>
#include <ser20/details/helpers.hpp>
#include <ser20/ser20.hpp>
#include <algorithm>
#include <exception>
#include <expected>
#include <sstream>
#include <string>
#include <type_traits>
#include <variant>
#include <vector>

//NOTE: a count off the wire is not trusted with an allocation - elements are read one by one, so a lie runs out of
//bytes and throws long before ser20's resize(count) would have zeroed gigabytes; the input archive is ours alone, and
//a container other than these two on the wire needs its own load here
namespace ser20
{
//NOTE: what is made room for ahead of the elements that back it
constexpr size_type kTrustedCount{256u};

template<class T, class A>
void SER20_LOAD_FUNCTION_NAME(PortableBinaryInputArchive& ar, std::vector<T, A>& vector)
	requires(!std::is_same_v<T, bool>)
{
	static_assert(!std::is_empty_v<T>, "an element of no bytes never runs out of them - a lied count reads forever");

	size_type size{};
	ar(make_size_tag(size));
	vector.clear();
	vector.reserve(std::min(size, kTrustedCount));
	for (size_type index{}; index < size; ++index)
	{
		ar(vector.emplace_back());
	}
}

template<class CharT, class Traits, class Alloc>
void SER20_LOAD_FUNCTION_NAME(PortableBinaryInputArchive& ar, std::basic_string<CharT, Traits, Alloc>& text)
{
	size_type size{};
	ar(make_size_tag(size));
	text.clear();
	for (size_type read{}; read < size;)
	{
		const size_type chunk{std::min(size - read, kTrustedCount)};
		text.resize(read + chunk);
		ar(binary_data(text.data() + read, chunk * sizeof(CharT)));
		read += chunk;
	}
}
}//namespace ser20

namespace network
{
namespace
{
//NOTE: names beside their types, so reordering the variant cannot mislabel the log; a missing name fails to compile
constexpr const char* NameOf(const commands::PositionChange&) { return "PositionChange"; }
constexpr const char* NameOf(const commands::TankShot&) { return "TankShot"; }
constexpr const char* NameOf(const commands::HealthChange&) { return "HealthChange"; }
constexpr const char* NameOf(const commands::TierChange&) { return "TierChange"; }
constexpr const char* NameOf(const commands::TankGrew&) { return "TankGrew"; }
constexpr const char* NameOf(const commands::Despawn&) { return "Despawn"; }
constexpr const char* NameOf(const commands::StatisticsChange&) { return "StatisticsChange"; }
constexpr const char* NameOf(const commands::KeyStateChange&) { return "KeyStateChange"; }
constexpr const char* NameOf(const commands::GameStateChange&) { return "GameStateChange"; }
constexpr const char* NameOf(const commands::BonusSpawn&) { return "BonusSpawn"; }
constexpr const char* NameOf(const commands::BonusStatus&) { return "BonusStatus"; }
constexpr const char* NameOf(const commands::BonusEffects&) { return "BonusEffects"; }
constexpr const char* NameOf(const commands::RespawnTank&) { return "RespawnTank"; }
constexpr const char* NameOf(const commands::RespawnCountChange&) { return "RespawnCountChange"; }
constexpr const char* NameOf(const commands::ObstacleSpawn&) { return "ObstacleSpawn"; }
constexpr const char* NameOf(const commands::TankSpawnComplete&) { return "TankSpawnComplete"; }
constexpr const char* NameOf(const commands::TankSpawnMoved&) { return "TankSpawnMoved"; }
constexpr const char* NameOf(const commands::BonusSpawnComplete&) { return "BonusSpawnComplete"; }
constexpr const char* NameOf(const commands::SignalEvent&) { return "SignalEvent"; }
constexpr const char* NameOf(const commands::SlotAssignment&) { return "SlotAssignment"; }
constexpr const char* NameOf(const commands::Disconnect&) { return "Disconnect"; }
constexpr const char* NameOf(const commands::WorldSnapshot&) { return "WorldSnapshot"; }
constexpr const char* NameOf(const commands::AbsenceChange&) { return "AbsenceChange"; }
constexpr const char* NameOf(const commands::PortForwardingChange&) { return "PortForwardingChange"; }
constexpr const char* NameOf(const commands::SeatHoldersChange&) { return "SeatHoldersChange"; }

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

std::string SerializeGoodbye(const DisconnectReason reason)
{
	commands::CommandBatch goodbye;
	goodbye.commands.emplace_back(commands::Disconnect{.reason = reason});

	return Serialize(goodbye);
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
