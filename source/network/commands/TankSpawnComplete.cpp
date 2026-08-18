#include "network/commands/TankSpawnComplete.h"

namespace network::commands
{
TankSpawnComplete::TankSpawnComplete(const Uuid uuid)
	: _uuid{uuid} {}

CommandType TankSpawnComplete::GetType() const noexcept { return _type; }

Uuid TankSpawnComplete::GetUuid() const noexcept { return _uuid; }

const char* TankSpawnComplete::GetClassNameW() const noexcept { return "TankSpawnComplete"; }
}//namespace network::commands
