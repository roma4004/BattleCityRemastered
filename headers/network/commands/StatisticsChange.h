#pragma once

#include "enums/CommandType.h"
#include "enums/StatisticsType.h"
#include <string>

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct StatisticsChange final
{
	CommandType type{CommandType::STATISTICS_CHANGE};
	StatisticsType statisticsType{};
	std::string author{};
	std::string fraction{};
};
}//namespace network::commands
