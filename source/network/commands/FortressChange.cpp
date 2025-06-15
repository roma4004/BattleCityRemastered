#include "../../../headers/network/commands/FortressChange.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(FortressChange);

FortressChange::FortressChange(): Command(CommandType::FORTRESS_CHANGE) {}

FortressChange::FortressChange(const std::string& state, const buuid uuid)
	: Command(CommandType::FORTRESS_CHANGE), _state(state), _uuid(uuid) {}

const std::string& FortressChange::GetState() const { return _state; }

using buuid = boost::uuids::uuid;
buuid FortressChange::GetUuid() const { return _uuid; }

const char* FortressChange::GetClassNameW() const { return "FortressChange"; }
