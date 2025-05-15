#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "../../../headers/network/commands/BonusDeSpawn.h"

BOOST_CLASS_EXPORT_IMPLEMENT(BonusDeSpawn);

BonusDeSpawn::BonusDeSpawn(): Command(CommandType::BONUS_DESPAWN) {}

BonusDeSpawn::BonusDeSpawn(const int id)
: Command(CommandType::BONUS_DESPAWN), _id(id) {}

int BonusDeSpawn::GetId() const { return _id; }

const char* BonusDeSpawn::GetClassNameW() const { return "BonusDeSpawn"; }

