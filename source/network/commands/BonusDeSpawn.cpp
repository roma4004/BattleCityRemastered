#include "network/commands/BonusDeSpawn.h"

namespace network::commands
{
BonusDeSpawn::BonusDeSpawn(const Uuid uuid)
	: _uuid{uuid} {}

CommandType BonusDeSpawn::GetType() const noexcept { return _type; }

Uuid BonusDeSpawn::GetUuid() const noexcept { return _uuid; }

const char* BonusDeSpawn::GetClassNameW() const noexcept { return "BonusDeSpawn"; }
}//namespace network::commands
