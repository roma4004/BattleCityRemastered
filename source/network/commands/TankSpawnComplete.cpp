#include "network/commands/TankSpawnComplete.h"

namespace network::commands
{
using buuid = boost::uuids::uuid;

TankSpawnComplete::TankSpawnComplete(const buuid uuid)
	: _uuid{uuid} {}

CommandType TankSpawnComplete::GetType() const noexcept { return _type; }

buuid TankSpawnComplete::GetUuid() const noexcept { return _uuid; }

const char* TankSpawnComplete::GetClassNameW() const noexcept { return "TankSpawnComplete"; }
}//namespace network::commands
