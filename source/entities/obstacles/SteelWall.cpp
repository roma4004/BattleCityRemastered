#include "entities/obstacles/SteelWall.h"
#include "components/EventSystem.h"
#include "enums/ObstacleType.h"

SteelWall::SteelWall(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const buuid uuid,
                     const GameMode gameMode)
	: Obstacle{rect,
	           0xaaaaaa,
	           1,
	           "SteelWall",
	           events,
	           uuid,
	           gameMode,
	           ObstacleType::Steel}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(false);

	Subscribe();
}

SteelWall::~SteelWall()
{
	Unsubscribe();
}

void SteelWall::Subscribe()
{
	_events->AddListener("Draw", _nameWithUuid, [this]() { this->Draw(); });
}

void SteelWall::Unsubscribe() const
{
	_events->RemoveListener("Draw", _nameWithUuid);
}
