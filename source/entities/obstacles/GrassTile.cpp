#include "entities/obstacles/GrassTile.h"
#include "components/EventSystem.h"
#include "enums/ObstacleType.h"

GrassTile::GrassTile(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const buuid uuid,
                     const GameMode gameMode)
	: Obstacle{rect,
	           0x1e90ff,
	           1,
	           "Grass",
	           events,
	           uuid,
	           gameMode,
	           ObstacleType::Grass}
{
	BaseObj::SetIsPassable(true);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(true);

	Subscribe();
}

GrassTile::~GrassTile()
{
	Unsubscribe();
}

void GrassTile::Subscribe()
{
	_events->AddListener("Draw", _nameWithUuid, [this]() { this->Draw(); });
}

void GrassTile::Unsubscribe() const
{
	_events->RemoveListener("Draw", _nameWithUuid);
}
