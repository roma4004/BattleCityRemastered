#include "entities/obstacles/IceTile.h"
#include "components/EventSystem.h"
#include "enums/ObstacleType.h"

IceTile::IceTile(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const buuid uuid,
                 const GameMode gameMode)
	: Obstacle{rect,
	           0x1e90ff,
	           1,
	           "Ice",
	           events,
	           uuid,
	           gameMode,
	           ObstacleType::Ice}
{
	BaseObj::SetIsPassable(true);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(true);

	Subscribe();
}

IceTile::~IceTile()
{
	Unsubscribe();
}

void IceTile::Subscribe()
{
	_events->AddListener("PreDraw", _nameWithUuid, [this]() { this->Draw(); });
}

void IceTile::Unsubscribe() const
{
	_events->RemoveListener("PreDraw", _nameWithUuid);
}
