#include "network/commands/TankShot.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(network::commands::TankShot);

namespace network::commands
{
TankShot::TankShot()
	: Command(CommandType::TANK_SHOT) {}

TankShot::TankShot(std::string who, const Direction dir, const buuid uuid)
	: Command{CommandType::TANK_SHOT}
	, _who{std::move(who)}
	, _dir{dir}
	, _uuid{uuid} {}

std::string TankShot::GetWho() const noexcept { return _who; }

Direction TankShot::GetDir() const noexcept { return _dir; }

using buuid = boost::uuids::uuid;
buuid TankShot::GetUuid() const noexcept { return _uuid; }

const char* TankShot::GetClassNameW() const noexcept { return "TankShot"; }
}//namespace network::commands
