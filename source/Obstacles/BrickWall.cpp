#include "../../headers/obstacles/BrickWall.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/ObstacleType.h"

#include <string>

struct Window;

BrickWall::BrickWall(ObjRectangle rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                     const boost::uuids::uuid uuid, const GameMode gameMode)
	: Obstacle{std::move(rect),
	           0x924b00,
	           1,
	           std::move(window),
	           {"BrickWall"},//TODO: change name for statistics
	           std::move(events),
	           uuid,
	           gameMode,
	           Brick}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);
}

void BrickWall::SendDamageStatistics(const std::string& author, const std::string& fraction)
{
	if (GetHealth() < 1)
	{
		//TODO: generalize who died in {_Name}Died and move method to Obstacle
		_events->EmitEvent<const std::string&, const std::string&>("BrickWallDied", author, fraction);//for statistic

		//TODO: move this to onHealthChange
		_events->EmitEvent<const std::string&, const int, const boost::uuids::uuid>(
				"ServerSend_Health", _name, GetHealth(), _uuid);//for replication
	}
}
