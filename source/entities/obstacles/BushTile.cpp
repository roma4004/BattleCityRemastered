#include "entities/obstacles/BushTile.h"
#include "components/EventSystem.h"
#include "enums/ObstacleType.h"
#include "application/GameConfig.h"

BushTile::BushTile(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const Uuid uuid,
				   const GameConfig& gameConfig)
	: Obstacle{rect, 1, events, uuid, gameConfig, ObstacleType::Bush, kCollision, kLayer}
{}

void BushTile::EmitDeathStatistics(Author) {}
