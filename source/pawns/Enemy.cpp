#include "../../headers/pawns/Enemy.h"
#include "../../headers/application/Window.h"
#include "../../headers/components/EventSystem.h"
#include "../../headers/enums/Direction.h"
#include "../../headers/enums/GameMode.h"
#include "../../headers/obstacles/FortressWall.h"
#include "../../headers/obstacles/WaterTile.h"
#include "../../headers/pawns/PawnProperty.h"
#include "../../headers/utils/TimeUtils.h"

#include <algorithm>

//TODO: if enemy see bullets they should try or prioritize move aside
Enemy::Enemy(PawnProperty pawnProperty, std::shared_ptr<BulletPool> bulletPool)
	: Bot{std::move(pawnProperty), std::move(bulletPool)} {}

Enemy::~Enemy() = default;

void Enemy::TickUpdate(const float deltaTime)
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
			if (_shootDistance > _bulletDamageRadius + _bulletOffset)//TODO: cover this by test
			{
				Shot();
			}
		}
	}

	if (_gameMode == PlayAsHost)
	{
		_events->EmitEvent<const std::string&, const FPoint, const Direction, const buuid&>(
				"ServerSend_Pos", _name, GetPos(), GetDirection(), _uuid);
	}
}
