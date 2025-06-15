#include "../../headers/obstacles/WaterTile.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/ObstacleType.h"

WaterTile::WaterTile(ObjRectangle rect, std::shared_ptr<EventSystem> events, const buuid uuid, const GameMode gameMode)
	: Obstacle{std::move(rect),
	           0x1e90ff,
	           1,
	           "Water",
	           std::move(events),
	           uuid,
	           gameMode,
	           Water}
{
	BaseObj::SetIsPassable(false);
	BaseObj::SetIsDestructible(false);
	BaseObj::SetIsPenetrable(true);
	
	_events->AddListener("Draw", _nameWithUuid, [this]() { this->Draw(this); });
	_events->AddListener<const float>("TickUpdate", _nameWithUuid, [this](const float /*deltaTime*/)
	{
		if (++_animFrameCounter; _animFrameCounter % 24 == 0)
		{
			// Animation with 1 px step and 1 frames
			++_animFrame;
			_animFrameCounter = 0;
			if (_animFrame > 8)
			{
				_animFrame = 0;
				_animFrameCounter = 0;
			}

			/* animation 8px step and 2 frames
			_animFrame = 0;
			if ( _animFrameCounter == 24)
			{
				_animFrame = 8;
				_animFrameCounter = 0;
			}
			*/

			// animation with 1 px step and 2 frame
			/*++_animFrame;
			_animFrameCounter = 0;
			if (_animFrame > 16)
			{
				_animFrame = 0;
				_animFrameCounter = 0;
			}*/
		}
		
	});
}

WaterTile::~WaterTile()
{
	_events->RemoveListener("Draw", _nameWithUuid);
}
