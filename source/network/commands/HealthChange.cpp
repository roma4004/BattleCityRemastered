#include "network/commands/HealthChange.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(HealthChange);

HealthChange::HealthChange(): Command(CommandType::HEALTH_CHANGE) {}

HealthChange::HealthChange(const std::string& who, const int health, const buuid uuid)
	: Command(CommandType::HEALTH_CHANGE), _who(who), _uuid(uuid), _health(health) {}

const std::string& HealthChange::GetWho() const { return _who; }

int HealthChange::GetHealth() const { return _health; }

using buuid = boost::uuids::uuid;
buuid HealthChange::GetUuid() const { return _uuid; }

const char* HealthChange::GetClassNameW() const { return "HealthChange"; }
