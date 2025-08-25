#include "network/commands/BonusSpawn.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(BonusSpawn);

BonusSpawn::BonusSpawn() : Command{CommandType::BONUS_SPAWN} {}

BonusSpawn::BonusSpawn(const FPoint pos, const BonusType bonusType, buuid uuid)
	: Command{CommandType::BONUS_SPAWN}, _pos{pos}, _bonusType{bonusType}, _uuid{std::move(uuid)} {}

FPoint BonusSpawn::GetPos() const noexcept { return _pos; }

BonusType BonusSpawn::GetBonusType() const noexcept { return _bonusType; }

using buuid = boost::uuids::uuid;
buuid BonusSpawn::GetUuid() const noexcept { return _uuid; }

const char* BonusSpawn::GetClassNameW() const noexcept { return "BonusSpawn"; }
