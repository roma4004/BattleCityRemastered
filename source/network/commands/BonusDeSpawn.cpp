#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

#include "../../../headers/network/commands/BonusDeSpawn.h"

BOOST_CLASS_EXPORT_IMPLEMENT(BonusDeSpawn);

BonusDeSpawn::BonusDeSpawn(): Command(CommandType::BONUS_DESPAWN) {}

BonusDeSpawn::BonusDeSpawn(const boost::uuids::uuid uuid)
	: Command(CommandType::BONUS_DESPAWN), _uuid(uuid) {}

boost::uuids::uuid BonusDeSpawn::GetUuid() const { return _uuid; }

const char* BonusDeSpawn::GetClassNameW() const { return "BonusDeSpawn"; }
