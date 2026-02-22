#include "network/commands/StatisticsChange.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(network::commands::StatisticsChange)

namespace network::commands
{
StatisticsChange::StatisticsChange()
	: Command{CommandType::STATISTICS_CHANGE} {}

StatisticsChange::StatisticsChange(std::string eventName, std::string author, std::string fraction)
	: Command{CommandType::STATISTICS_CHANGE}
	, _eventName{std::move(eventName)}
	, _author{std::move(author)}
	, _fraction{std::move(fraction)} {}

std::string StatisticsChange::GetEventName() const noexcept { return _eventName; }

std::string StatisticsChange::GetAuthor() const noexcept { return _author; }

std::string StatisticsChange::GetFraction() const noexcept { return _fraction; }

const char* StatisticsChange::GetClassNameW() const noexcept { return "StatisticsChange"; }
}//namespace network::commands
