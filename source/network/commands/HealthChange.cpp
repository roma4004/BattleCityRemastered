#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "../../../headers/network/commands/HealthChange.h"

BOOST_CLASS_EXPORT_IMPLEMENT(HealthChange);

HealthChange::HealthChange(): Command(CommandType::HEALTH_CHANGE) {}

HealthChange::HealthChange(const std::string& who, const int health, const boost::uuids::uuid uuid)
	: Command(CommandType::HEALTH_CHANGE), _who(who), _health(health), _uuid(uuid) {}

const std::string& HealthChange::GetWho() const { return _who; }

int HealthChange::GetHealth() const { return _health; }

boost::uuids::uuid HealthChange::GetUuid() const { return _uuid; }

const char* HealthChange::GetClassNameW() const { return "HealthChange"; }
