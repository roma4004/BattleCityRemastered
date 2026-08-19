#pragma once

#include "geometry/ObjRectangle.h"
#include "enums/CommandType.h"
#include "enums/TankType.h"
#include "utils/Uuid.h"

namespace network::commands
{
//NOTE: wire DTO - serialization lives in CommandSerialization.h
struct RespawnTank final
{
	CommandType type{CommandType::RESPAWN_TANK};
	TankType tankType{};
	Uuid uuid{};
	ObjRectangle rect{};
};
}//namespace network::commands
