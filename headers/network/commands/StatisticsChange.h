#pragma once

#include "enums/CommandType.h"
#include "enums/StatisticsType.h"
#include "utils/Uuid.h"
#include <string>

enum class Faction : char8_t;

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct StatisticsChange final
{
	CommandType type{CommandType::STATISTICS_CHANGE};
	StatisticsType statisticsType{};
	//NOTE: only the tank facts carry it - which counter they land in is the receiver's call, not the wire's
	std::string who{};
	std::string author{};
	Faction faction{};
	//NOTE: only TankDied carries it - a name is not identity
	Uuid uuid{};
};
}//namespace network::commands
