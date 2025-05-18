#include "../../headers/obstacles/SteelWall.h"
#include "../../headers/components/EventSystem.h"

#include <string>

SteelWall::SteelWall(const ObjRectangle& rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                     const boost::uuids::uuid uuid, const GameMode gameMode)
	: Obstacle{{.x = rect.x, .y = rect.y, .w = rect.w - 1, .h = rect.h - 1},
	           0xaaaaaa,
	           1,
	           std::move(window),
	           "SteelWall",//TODO: change name for statistics
	           std::move(events),
	           uuid,
	           gameMode}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(false);
}

void SteelWall::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	if (GetHealth() < 1)
	{
		_events->EmitEvent<const std::string&, const std::string&>("SteelWallDied", author, fraction);
	}
}
