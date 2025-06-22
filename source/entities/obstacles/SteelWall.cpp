#include "entities/obstacles/SteelWall.h"
#include "components/EventSystem.h"
#include "enums/ObstacleType.h"

SteelWall::SteelWall(const ObjRectangle rect, std::shared_ptr<EventSystem> events, const buuid uuid,
                     const GameMode gameMode)
	: Obstacle{rect,
	           0xaaaaaa,
	           1,
	           "SteelWall",
	           std::move(events),
	           uuid,
	           gameMode,
	           Steel}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(false);
}

SteelWall::~SteelWall() = default;
