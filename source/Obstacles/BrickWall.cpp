#include "../../headers/obstacles/BrickWall.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/ObstacleType.h"

BrickWall::BrickWall(const ObjRectangle rect, std::shared_ptr<EventSystem> events, const buuid uuid, const GameMode gameMode)
	: Obstacle{rect,
	           0x924b00,
	           1,
	           {"BrickWall"},
	           std::move(events),
	           uuid,
	           gameMode,
	           Brick}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);
}

BrickWall::~BrickWall() = default;
