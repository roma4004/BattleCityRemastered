#include "../../headers/obstacles/IceTile.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/ObstacleType.h"

IceTile::IceTile(ObjRectangle rect, std::shared_ptr<EventSystem> events, const buuid uuid, const GameMode gameMode)
	: Obstacle{std::move(rect),
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

	_events->AddListener("Draw", _nameWithUuid, [this]() { this->Draw(this); });
}

IceTile::~IceTile()
{
	_events->RemoveListener("Draw", _nameWithUuid);
}
