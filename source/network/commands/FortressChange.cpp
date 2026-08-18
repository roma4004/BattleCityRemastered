#include "network/commands/FortressChange.h"

namespace network::commands
{
FortressChange::FortressChange(const FortressState state, const Uuid uuid)
	: _state{state}
	, _uuid{uuid} {}

CommandType FortressChange::GetType() const noexcept { return _type; }

FortressState FortressChange::GetState() const noexcept { return _state; }

Uuid FortressChange::GetUuid() const noexcept { return _uuid; }

const char* FortressChange::GetClassNameW() const noexcept { return "FortressChange"; }
}//namespace network::commands
