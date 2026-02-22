#include "entities/obstacles/BrickWall.h"
#include "components/EventSystem.h"
#include "enums/ObstacleType.h"

BrickWall::BrickWall(const ObjRectangle rect, const std::shared_ptr<EventSystem>& events, const buuid uuid,
					 const GameMode gameMode)
	: Obstacle{rect,
			   0x924b00,
			   1,
			   {"BrickWall"},
			   events,
			   uuid,
			   gameMode,
			   ObstacleType::Brick}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(true);
	BaseObj::SetIsPenetrable(false);

	Subscribe();
}

BrickWall::~BrickWall()
{
	Unsubscribe();
}

void BrickWall::Subscribe()
{
	_events->AddListener("Draw", _nameWithUuid, [this]() { this->Draw(); });
}

void BrickWall::Unsubscribe() const
{
	_events->RemoveListener("Draw", _nameWithUuid);
}
