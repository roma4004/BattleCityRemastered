#include "../../../headers/network/commands/StatisticsChange.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(StatisticsChange);

StatisticsChange::StatisticsChange(): Command(CommandType::STATISTICS_CHANGE) {}

StatisticsChange::StatisticsChange(const std::string& eventName, const std::string& author, const std::string& fraction)
	: Command(CommandType::STATISTICS_CHANGE), _eventName(eventName), _author(author), _fraction(fraction) {}

const std::string& StatisticsChange::GetEventName() const { return _eventName; }
const std::string& StatisticsChange::GetAuthor() const { return _author; }
const std::string& StatisticsChange::GetFraction() const { return _fraction; }

const char* StatisticsChange::GetClassNameW() const { return "StatisticsChange"; }
