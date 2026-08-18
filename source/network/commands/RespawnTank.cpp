#include "network/commands/RespawnTank.h"

namespace network::commands
{
RespawnTank::RespawnTank(const TankType tankType, const Uuid uuid, const ObjRectangle rect)
	: _tankType{tankType}
	, _uuid{uuid}
	, _rect{rect} {}

CommandType RespawnTank::GetType() const noexcept { return _type; }

Uuid RespawnTank::GetUuid() const noexcept { return _uuid; }

TankType RespawnTank::GetTankType() const noexcept { return _tankType; }

ObjRectangle RespawnTank::GetRect() const noexcept { return _rect; }

const char* RespawnTank::GetClassNameW() const noexcept { return "RespawnTank"; }
}//namespace network::commands
