#pragma once

#include "enums/Author.h"
#include "enums/CommandType.h"
#include "enums/StatisticsType.h"
#include "utils/Uuid.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct StatisticsChange final
{
	CommandType type{CommandType::STATISTICS_CHANGE};
	StatisticsType statisticsType{};
	//NOTE: only the tank facts carry it - which counter they land in is the receiver's call, not the wire's
	Author who{};
	Author author{};
	//NOTE: only TankDied carries it - a name is not identity
	Uuid uuid{};
};
}//namespace network::commands
