#include "network/commands/HealthChange.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(network::commands::HealthChange)

namespace network::commands
{
HealthChange::HealthChange()
	: Command{CommandType::HEALTH_CHANGE} {}

HealthChange::HealthChange(std::string who, const int health, const buuid uuid)
	: Command{CommandType::HEALTH_CHANGE}
	, _who{std::move(who)}
	, _health{health}
	, _uuid{uuid} {}

std::string HealthChange::GetWho() const noexcept { return _who; }

int HealthChange::GetHealth() const noexcept { return _health; }

using buuid = boost::uuids::uuid;
buuid HealthChange::GetUuid() const noexcept { return _uuid; }

const char* HealthChange::GetClassNameW() const noexcept { return "HealthChange"; }
}//namespace network::commands
