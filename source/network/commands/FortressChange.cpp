#include "network/commands/FortressChange.h"

namespace network::commands
{
FortressChange::FortressChange(std::string state, const buuid uuid)
	: _state{std::move(state)}
	, _uuid{uuid} {}

CommandType FortressChange::GetType() const noexcept { return _type; }

std::string FortressChange::GetState() const noexcept { return _state; }

using buuid = boost::uuids::uuid;
buuid FortressChange::GetUuid() const noexcept { return _uuid; }

const char* FortressChange::GetClassNameW() const noexcept { return "FortressChange"; }
}//namespace network::commands
