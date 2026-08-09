#include "entities/obstacles/IceTile.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "enums/ObstacleType.h"

IceTile::IceTile(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const buuid uuid,
				 const GameMode gameMode)
	: Obstacle{rect, 1, "Ice", events, uuid, gameMode, ObstacleType::Ice}
{
	BaseObj::SetIsPassable(true);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(true);

	Subscribe();
}

void IceTile::Subscribe()
{
	_subs.push_back(_events->AddListener(_nameWithUuid, [this](const PreDrawEvent&) { this->Draw(); }));
}

void IceTile::EmitDeathStatistics(const std::string&, const std::string&) {}
