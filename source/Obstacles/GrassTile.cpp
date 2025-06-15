#include "../../headers/obstacles/GrassTile.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/ObstacleType.h"

GrassTile::GrassTile(ObjRectangle rect, std::shared_ptr<EventSystem> events, const buuid uuid, const GameMode gameMode)
	: Obstacle{std::move(rect),
	           0x1e90ff,
	           1,
	           "Grass",
	           std::move(events),
	           uuid,
	           gameMode,
	           Grass}
{
	BaseObj::SetIsPassable(true);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(true);

	_events->AddListener("Draw", _nameWithUuid, [this]() { this->Draw(this); });
}

GrassTile::~GrassTile()
{
	_events->RemoveListener("Draw", _nameWithUuid);
}
