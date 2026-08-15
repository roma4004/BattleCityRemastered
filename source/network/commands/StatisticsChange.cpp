#include "network/commands/StatisticsChange.h"

namespace network::commands
{
StatisticsChange::StatisticsChange(const StatisticsType type, std::string author, std::string fraction)
	: _statisticsType{type}
	, _author{std::move(author)}
	, _fraction{std::move(fraction)} {}

CommandType StatisticsChange::GetType() const noexcept { return _type; }

StatisticsType StatisticsChange::GetStatisticsType() const noexcept { return _statisticsType; }

std::string StatisticsChange::GetAuthor() const noexcept { return _author; }

std::string StatisticsChange::GetFraction() const noexcept { return _fraction; }

const char* StatisticsChange::GetClassNameW() const noexcept { return "StatisticsChange"; }
}//namespace network::commands
