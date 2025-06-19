#include "../../headers/obstacles/IceTile.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/ObstacleType.h"

IceTile::IceTile(const ObjRectangle rect, std::shared_ptr<EventSystem> events, const buuid uuid,
                 const GameMode gameMode)
	: Obstacle{rect,
	           0x1e90ff,
	           1,
	           "Ice",
	           std::move(events),
	           uuid,
	           gameMode,
	           Ice}
{
	BaseObj::SetIsPassable(true);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(true);
}

IceTile::~IceTile() = default;
