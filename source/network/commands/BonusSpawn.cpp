#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "../../../headers/network/commands/BonusSpawn.h"

BOOST_CLASS_EXPORT_IMPLEMENT(BonusSpawn);

BonusSpawn::BonusSpawn(): Command(CommandType::BONUS_SPAWN) {}

BonusSpawn::BonusSpawn(const FPoint& pos, const BonusType bonusType, const int id)
: Command(CommandType::BONUS_SPAWN), _pos(pos), _bonusType(bonusType), _id(id) {}

FPoint BonusSpawn::GetPos() const { return _pos; }

BonusType BonusSpawn::GetBonusType() const { return _bonusType; }

int BonusSpawn::GetId() const { return _id; }

const char* BonusSpawn::GetClassNameW() const { return "BonusSpawn"; }

