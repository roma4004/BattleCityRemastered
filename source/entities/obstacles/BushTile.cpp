#include "entities/obstacles/BushTile.h"
#include "components/EventSystem.h"
#include "enums/ObstacleType.h"

BushTile::BushTile(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const buuid uuid,
					 const GameMode gameMode)
	: Obstacle{rect, 1, "Bush", events, uuid, gameMode, ObstacleType::Bush}
{
	BaseObj::SetIsPassable(true);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(true);

	Subscribe();
}

BushTile::~BushTile()
{
	Unsubscribe();
}

void BushTile::Subscribe()
{
	_events->AddListener("PostDraw", _nameWithUuid, [this]() { this->Draw(); });
}

void BushTile::Unsubscribe() const { _events->RemoveAllListeners(_nameWithUuid); }
