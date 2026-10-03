#include "entities/obstacles/IceTile.h"
#include "components/EventSystem.h"
#include "enums/ObstacleType.h"
#include "application/GameConfig.h"

IceTile::IceTile(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const Uuid uuid,
				 const GameConfig& gameConfig)
	: Obstacle{rect, 1, events, uuid, gameConfig, ObstacleType::Ice, kCollision, kLayer}
{}

void IceTile::EmitDeathStatistics(Author) {}
