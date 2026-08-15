#include "network/commands/BonusDeSpawn.h"

namespace network::commands
{
BonusDeSpawn::BonusDeSpawn(const buuid uuid)
	: _uuid{uuid} {}

CommandType BonusDeSpawn::GetType() const noexcept { return _type; }

using buuid = boost::uuids::uuid;
buuid BonusDeSpawn::GetUuid() const noexcept { return _uuid; }

const char* BonusDeSpawn::GetClassNameW() const noexcept { return "BonusDeSpawn"; }
}//namespace network::commands
