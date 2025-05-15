#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "../../../headers/network/commands/TankShot.h"

BOOST_CLASS_EXPORT_IMPLEMENT(TankShot);

TankShot::TankShot(): Command(CommandType::TANK_SHOT) {}

TankShot::TankShot(const std::string& who, const Direction dir)
: Command(CommandType::TANK_SHOT), _who(who), _dir(dir) {}

const std::string& TankShot::GetWho() const { return _who; }

Direction TankShot::GetDir() const { return _dir; }

const char* TankShot::GetClassNameW() const { return "TankShot"; }
