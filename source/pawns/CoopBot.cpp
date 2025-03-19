#include "../../headers/pawns/CoopBot.h"
#include "../../headers/application/Window.h"
#include "../../headers/components/LineOfSight.h"
#include "../../headers/enums/Direction.h"
#include "../../headers/enums/GameMode.h"
#include "../../headers/obstacles/FortressWall.h"
#include "../../headers/obstacles/WaterTile.h"
#include "../../headers/utils/TimeUtils.h"

#include <algorithm>

CoopBot::CoopBot(const ObjRectangle& rect, const int color, const int health, std::shared_ptr<Window> window,
                 const Direction dir, const float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
                 std::shared_ptr<EventSystem> events, std::string name, std::string fraction,
                 std::shared_ptr<BulletPool> bulletPool, const GameMode gameMode, const int id, const int tier)
	: Bot{rect,
	      color,
	      health,
	      std::move(window),
	      dir,
	      speed,
	      allObjects,
	      std::move(events),
	      std::move(name),
	      std::move(fraction),
	      std::move(bulletPool),
	      gameMode,
	      id,
	      tier} {}

CoopBot::~CoopBot() = default;

void CoopBot::TickUpdate(const float deltaTime)
{
	if (!this->_timer.isActive)
	{
		this->Bot::TickUpdate(deltaTime);
	}

	// shot
	if (TimeUtils::IsCooldownFinish(_lastTimeFire, _fireCooldown))
	{
		HandleLineOfSight(GetDirection());

		if (_nearestSeenObstacle
		    && (_nearestSeenObstacle->GetIsDestructible() || _tier > 2)
		    && !dynamic_cast<WaterTile*>(_nearestSeenObstacle.get())
		    // && !dynamic_cast<BushesTile*>(_nearestSeenObstacle.get())
		    // && !dynamic_cast<IceTile*>(_nearestSeenObstacle.get())
		    && !dynamic_cast<FortressWall*>(_nearestSeenObstacle.get())
		    && !IsAlly(_nearestSeenObstacle))
		{
			if (_shootDistance > _bulletDamageRadius + _bulletOffset)
			{
				Shot();
			}
		}
	}
}
