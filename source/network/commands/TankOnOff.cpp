#include "network/commands/TankOnOff.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>//NOTE: required for serialization uuid
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(network::commands::TankOnOff)

namespace network::commands
{
TankOnOff::TankOnOff()
	: Command{CommandType::TANK_ON_OFF} {}

TankOnOff::TankOnOff(const buuid uuid, const bool isEnable, std::string name)
	: Command{CommandType::TANK_ON_OFF}
	, _uuid{uuid}
	, _isEnable{isEnable}
	, _name{std::move(name)} {}

using buuid = boost::uuids::uuid;
buuid TankOnOff::GetUuid() const noexcept { return _uuid; }
bool TankOnOff::GetIsEnable() const noexcept { return _isEnable; }
std::string TankOnOff::GetName() const noexcept { return _name; }

const char* TankOnOff::GetClassNameW() const noexcept { return "TankOnOff"; }
}//namespace network::commands
