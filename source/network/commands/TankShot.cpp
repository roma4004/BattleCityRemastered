#include "network/commands/TankShot.h"

namespace network::commands
{
TankShot::TankShot(std::string who, const Direction dir, const Uuid uuid)
	: _who{std::move(who)}
	, _dir{dir}
	, _uuid{uuid} {}

CommandType TankShot::GetType() const noexcept { return _type; }

std::string TankShot::GetWho() const noexcept { return _who; }

Direction TankShot::GetDir() const noexcept { return _dir; }

Uuid TankShot::GetUuid() const noexcept { return _uuid; }

const char* TankShot::GetClassNameW() const noexcept { return "TankShot"; }
}//namespace network::commands
