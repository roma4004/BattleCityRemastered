#include "../../headers/obstacles/SteelWall.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/ObstacleType.h"

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
