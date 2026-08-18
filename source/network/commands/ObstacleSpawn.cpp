#include "network/commands/ObstacleSpawn.h"
#include "entities/ObjRectangle.h"
#include "enums/ObstacleType.h"

namespace network::commands
{
ObstacleSpawn::ObstacleSpawn(const ObjRectangle rect, const ObstacleType obstacleType, const Uuid uuid)
	: _rect{rect}
	, _obstacleType{obstacleType}
	, _uuid{uuid} {}

CommandType ObstacleSpawn::GetType() const noexcept { return _type; }

ObjRectangle ObstacleSpawn::GetRect() const noexcept { return _rect; }

ObstacleType ObstacleSpawn::GetObstacleType() const noexcept { return _obstacleType; }

Uuid ObstacleSpawn::GetUuid() const noexcept { return _uuid; }

const char* ObstacleSpawn::GetClassNameW() const noexcept { return "ObstacleSpawn"; }
}//namespace network::commands
