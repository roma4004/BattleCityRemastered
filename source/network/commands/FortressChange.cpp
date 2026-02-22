#include "network/commands/FortressChange.h"
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include <boost/serialization/export.hpp>
#include <boost/serialization/shared_ptr.hpp>

BOOST_CLASS_EXPORT_IMPLEMENT(network::commands::FortressChange)

namespace network::commands
{
FortressChange::FortressChange()
	: Command{CommandType::FORTRESS_CHANGE} {}

FortressChange::FortressChange(std::string state, const buuid uuid)
	: Command{CommandType::FORTRESS_CHANGE}
	, _state{std::move(state)}
	, _uuid{uuid} {}

std::string FortressChange::GetState() const noexcept { return _state; }

using buuid = boost::uuids::uuid;
buuid FortressChange::GetUuid() const noexcept { return _uuid; }

const char* FortressChange::GetClassNameW() const noexcept { return "FortressChange"; }
}//namespace network::commands
