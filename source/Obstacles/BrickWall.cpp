#include "../../headers/obstacles/BrickWall.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/ObstacleType.h"

#include <string>

struct Window;

BrickWall::BrickWall(ObjRectangle rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                     const buuid uuid, const GameMode gameMode)
	: Obstacle{std::move(rect),
	           0x924b00,
	           1,
	           std::move(window),
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
