#include "../../headers/obstacles/SteelWall.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/ObstacleType.h"

#include <string>

SteelWall::SteelWall(ObjRectangle rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                     const boost::uuids::uuid uuid, const GameMode gameMode)
	: Obstacle{std::move(rect),
	           0xaaaaaa,
	           1,
	           std::move(window),
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

void SteelWall::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	if (GetHealth() < 1)
	{
		_events->EmitEvent<const std::string&, const std::string&>("Statistics_SteelWallDied", author, fraction);
	}
}
