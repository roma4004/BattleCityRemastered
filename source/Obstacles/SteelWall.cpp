#include "../../headers/obstacles/SteelWall.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/ObstacleType.h"

#include <string>

SteelWall::SteelWall(ObjRectangle rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                     const buuid uuid, const GameMode gameMode)
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

	//TODO: remove this after load steel texture
	_events->AddListener("Draw", _nameWithUuid, [this]() { this->Draw(this); });
}

SteelWall::~SteelWall()
{
	//TODO: remove this after load steel texture
	_events->RemoveListener("Draw", _nameWithUuid);
}
