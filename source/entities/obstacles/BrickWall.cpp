#include "entities/obstacles/BrickWall.h"
#include "components/EventSystem.h"
#include "enums/ObstacleType.h"

BrickWall::BrickWall(const ObjRectangle rect, std::shared_ptr<EventSystem> events, const buuid uuid,
                     const GameMode gameMode)
	: Obstacle{rect,
	           0x924b00,
	           1,
	           {"BrickWall"},
	           std::move(events),
	           uuid,
	           gameMode,
	           ObstacleType::Brick}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);
}

BrickWall::BrickWall(const ObjRectangle rect, std::shared_ptr<EventSystem> events, const buuid uuid,
                     const GameMode gameMode, const bool isReplicationOn)
	: Obstacle{rect,
	           0x924b00,
	           1,
	           {"BrickWall"},
	           std::move(events),
	           uuid,
	           gameMode,
	           ObstacleType::Brick,
	           isReplicationOn}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);
}

BrickWall::~BrickWall() = default;
