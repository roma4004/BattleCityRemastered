#include "entities/pawns/Enemy.h"
#include "entities/pawns/PawnProperty.h"
#include "utils/TimeUtils.h"
#include <algorithm>

//TODO: fix enemy stuck in bricks(local game) looks like 1 pixel issue when finding free path
//TODO: if enemy see bullets they should try or prioritize move aside
Enemy::Enemy(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, const bool enableByDefault)
	: Bot{std::move(pawnProperty), bulletPool, enableByDefault} {}

Enemy::~Enemy() = default;

void Enemy::TickUpdate(const double deltaTime)
{
	if (_effects.isTimerActive)//TODO: disable tick update when active
	{
		return;
	}

	Bot::TickUpdate(deltaTime);

	// shot
	if (TimeUtils::IsCooldownFinish(_lastTimeFire, _fireCooldown))
	{
		const std::shared_ptr<BaseObj> nearestSeenObstacle = HandleLineOfSight(GetDirection());

		if (!nearestSeenObstacle || IsAlly(nearestSeenObstacle))
		{
			return;
		}

		if (const auto obstacle = nearestSeenObstacle.get();
			obstacle
			&& (obstacle->GetIsDestructible() || _tier > 2)
			&& !obstacle->GetIsPenetrable())// skip water, ice, bush(Grass)
		{
			if (_shootDistance > _bulletDamageRadius + _bulletOffset)//TODO: cover this by test
			{
				Shot();
			}
		}
	}
}
