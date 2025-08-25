#include "network/commands/RespawnTank.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(RespawnTank);

RespawnTank::RespawnTank() : Command{CommandType::RESPAWN_TANK} {}

RespawnTank::RespawnTank(const TankType tankType, buuid uuid)
	: Command{CommandType::RESPAWN_TANK}, _tankType{tankType}, _uuid{std::move(uuid)} {}

using buuid = boost::uuids::uuid;
buuid RespawnTank::GetUuid() const noexcept { return _uuid; }

TankType RespawnTank::GetTankType() const noexcept { return _tankType; }

const char* RespawnTank::GetClassNameW() const noexcept { return "RespawnTank"; }
