#include "../../../headers/network/commands/TankShot.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(TankShot);

TankShot::TankShot(): Command(CommandType::TANK_SHOT) {}

TankShot::TankShot(const std::string& who, const Direction dir, const buuid uuid)
	: Command(CommandType::TANK_SHOT), _who(who), _dir(dir), _uuid(uuid) {}

const std::string& TankShot::GetWho() const { return _who; }

Direction TankShot::GetDir() const { return _dir; }

using buuid = boost::uuids::uuid;
buuid TankShot::GetUuid() const { return _uuid; }

const char* TankShot::GetClassNameW() const { return "TankShot"; }
