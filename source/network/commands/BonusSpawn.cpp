#include "network/commands/BonusSpawn.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(BonusSpawn);

BonusSpawn::BonusSpawn(): Command(CommandType::BONUS_SPAWN) {}

BonusSpawn::BonusSpawn(const FPoint& pos, const BonusType bonusType, const buuid uuid)
	: Command(CommandType::BONUS_SPAWN), _pos(pos), _uuid(uuid), _bonusType(bonusType) {}

FPoint BonusSpawn::GetPos() const { return _pos; }

BonusType BonusSpawn::GetBonusType() const { return _bonusType; }

using buuid = boost::uuids::uuid;
buuid BonusSpawn::GetUuid() const { return _uuid; }

const char* BonusSpawn::GetClassNameW() const { return "BonusSpawn"; }
