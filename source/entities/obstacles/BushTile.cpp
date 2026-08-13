#include "entities/obstacles/BushTile.h"
#include "components/EventSystem.h"
#include "components/events/CoreLifecycleEvents.h"
#include "enums/ObstacleType.h"

BushTile::BushTile(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const buuid uuid,
				   const GameMode gameMode)
	: Obstacle{rect, 1, "Bush", events, uuid, gameMode, ObstacleType::Bush, s_collision}
{
	Subscribe();
}

void BushTile::Subscribe()
{
	_subs.push_back(_events->AddListener(this, &BushTile::OnPostDraw));
}

void BushTile::OnPostDraw(const PostDrawEvent&) const { Draw(); }

void BushTile::EmitDeathStatistics(const std::string&, const std::string&) {}
