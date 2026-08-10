#include "entities/obstacles/IceTile.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "enums/ObstacleType.h"

IceTile::IceTile(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const buuid uuid,
				 const GameMode gameMode)
	: Obstacle{rect, 1, "Ice", events, uuid, gameMode, ObstacleType::Ice, kCollision}
{
	Subscribe();
}

void IceTile::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &IceTile::OnPreDraw));
}

void IceTile::OnPreDraw(const PreDrawEvent&) { Draw(); }

void IceTile::EmitDeathStatistics(const std::string&, const std::string&) {}
