#include "../../headers/obstacles/WaterTile.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/ObstacleType.h"

#include <string>

WaterTile::WaterTile(ObjRectangle rect, std::shared_ptr<Window> window, std::shared_ptr<EventSystem> events,
                     const buuid uuid, const GameMode gameMode, std::shared_ptr<IDrawable> textureManager)
	: Obstacle{std::move(rect),
	           0x1e90ff,
	           1,
	           std::move(window),
	           "Water",
	           std::move(events),
	           uuid,
	           gameMode,
	           Water,
	           std::move(textureManager)}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(true);

	//TODO: remove this after load steel texture
	_events->AddListener("Draw", _nameWithUuid, [this]() { this->Draw(this); });
}

WaterTile::~WaterTile()
{
	//TODO: remove this after load steel texture
	_events->RemoveListener("Draw", _nameWithUuid);
}

void WaterTile::SendDamageStatistics(const std::string& /*author*/, const std::string& /*fraction*/) {}
