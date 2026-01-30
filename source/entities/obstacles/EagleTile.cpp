#include "entities/obstacles/EagleTile.h"
#include "components/EventSystem.h"
#include "enums/ObstacleType.h"

EagleTile::EagleTile(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const buuid uuid,
                     const GameMode gameMode)
	: Obstacle{rect,
	           0x1e90ff,
	           1,
	           "Eagle",
	           events,
	           uuid,
	           gameMode,
	           ObstacleType::Eagle}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	Subscribe();
}

EagleTile::~EagleTile()
{
	Unsubscribe();

	_events->EmitEvent("PlayersBaseFinished");
}

void EagleTile::Subscribe()
{
	_events->AddListener("Draw", _nameWithUuid, [this]() { this->Draw(); });
}

void EagleTile::Unsubscribe() const
{
	_events->RemoveListener("Draw", _nameWithUuid);
}
