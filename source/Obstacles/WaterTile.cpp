#include "../../headers/obstacles/WaterTile.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/ObstacleType.h"

WaterTile::WaterTile(ObjRectangle rect, std::shared_ptr<EventSystem> events, const buuid uuid, const GameMode gameMode)
	: Obstacle{std::move(rect),
	           0x1e90ff,
	           1,
	           "Water",
	           std::move(events),
	           uuid,
	           gameMode,
	           Water}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(true);

	//TODO: remove this after load steel texture
	_events->AddListener("Draw", _nameWithUuid, [this]() { this->Draw(this); });
}

WaterTile::~WaterTile()
{
	//TODO: remove this after load steel texture
	_events->RemoveListener("Draw", _nameWithUuid);
}
