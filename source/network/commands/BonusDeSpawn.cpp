#include "network/commands/BonusDeSpawn.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>//NOTE: required for serialization uuid
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(BonusDeSpawn);

BonusDeSpawn::BonusDeSpawn(): Command(CommandType::BONUS_DESPAWN) {}

BonusDeSpawn::BonusDeSpawn(buuid uuid)
	: Command(CommandType::BONUS_DESPAWN), _uuid(std::move(uuid)) {}

using buuid = boost::uuids::uuid;
buuid BonusDeSpawn::GetUuid() const noexcept { return _uuid; }

const char* BonusDeSpawn::GetClassNameW() const noexcept { return "BonusDeSpawn"; }
