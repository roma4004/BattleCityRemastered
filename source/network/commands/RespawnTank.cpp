#include "../../../headers/network/commands/RespawnTank.h"

#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(RespawnTank);

RespawnTank::RespawnTank(): Command(CommandType::RESPAWN_TANK) {}

RespawnTank::RespawnTank(const TankType tankType, const boost::uuids::uuid uuid)
	: Command(CommandType::RESPAWN_TANK), _tankType(tankType), _uuid(uuid) {}

boost::uuids::uuid RespawnTank::GetUuid() const { return _uuid; }

TankType RespawnTank::GetTankType() const { return _tankType; }

const char* RespawnTank::GetClassNameW() const { return "BonusDeSpawn"; }
