#include "../../headers/pawns/CoopBot.h"
#include "../../headers/obstacles/FortressWall.h"
#include "../../headers/obstacles/WaterTile.h"
#include "../../headers/pawns/PawnProperty.h"
#include "../../headers/utils/TimeUtils.h"

#include <algorithm>

CoopBot::CoopBot(PawnProperty pawnProperty, std::shared_ptr<BulletPool> bulletPool)
	: Bot{std::move(pawnProperty), std::move(bulletPool)} {}

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
		const std::shared_ptr<BaseObj> nearestSeenObstacle = HandleLineOfSight(GetDirection());

		if (nearestSeenObstacle && nearestSeenObstacle.get() != nullptr
		    && (nearestSeenObstacle->GetIsDestructible() || _tier > 2)
		    && !dynamic_cast<WaterTile*>(nearestSeenObstacle.get())
		    // && !dynamic_cast<BushesTile*>(nearestSeenObstacle.get())
		    // && !dynamic_cast<IceTile*>(nearestSeenObstacle.get())
		    && !dynamic_cast<FortressWall*>(nearestSeenObstacle.get())
		    && !IsAlly(nearestSeenObstacle))
		{
			if (_shootDistance > _bulletDamageRadius + _bulletOffset)
			{
				Shot();
			}
		}
	}
}
