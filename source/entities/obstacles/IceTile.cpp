#include "entities/obstacles/IceTile.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "enums/ObstacleType.h"
#include "application/GameConfig.h"

IceTile::IceTile(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const Uuid uuid,
				 const GameConfig& gameConfig)
	: Obstacle{rect, 1, events, uuid, gameConfig, ObstacleType::Ice, kCollision}
{}

void IceTile::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &IceTile::OnPreDraw));
}

void IceTile::OnPreDraw(const PreDrawEvent&) const { Draw(); }

void IceTile::EmitDeathStatistics(Author) {}
