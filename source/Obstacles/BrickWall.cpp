#include "../../headers/obstacles/BrickWall.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/ObstacleType.h"

#include <string>

struct Window;

BrickWall::BrickWall(ObjRectangle rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                     const boost::uuids::uuid uuid, const GameMode gameMode,
                     std::shared_ptr<SDL_Texture> textureCollection,
                     std::shared_ptr<SDL_Renderer> renderer)
	: Obstacle{std::move(rect),
	           0x924b00,
	           1,
	           std::move(window),
	           {"BrickWall"},
	           std::move(events),
	           uuid,
	           gameMode,
	           Brick,
	           std::move(textureCollection),
	           std::move(renderer)}
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
		_events->EmitEvent<const std::string&, const std::string&>("Statistics_BrickWallDied", author, fraction);

		//TODO: move this to onHealthChange
		_events->EmitEvent<const std::string&, const int, const boost::uuids::uuid>(
				"ServerSend_Health", _name, GetHealth(), _uuid);//for replication
	}
}
