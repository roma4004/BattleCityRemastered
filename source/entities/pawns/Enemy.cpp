#include "entities/pawns/Enemy.h"
#include "entities/pawns/PawnProperty.h"
#include "utils/TimeUtils.h"
#include <algorithm>

//TODO: fix enemy stuck in bricks(local game)
//TODO: if enemy see bullets they should try or prioritize move aside
Enemy::Enemy(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool,
             const BonusEffectProperty effects, const bool enableByDefault)
	: Bot{std::move(pawnProperty), bulletPool, effects, enableByDefault} {}

Enemy::~Enemy() = default;

void Enemy::TickUpdate(const float deltaTime)
{
	if (_effects.isTimerActive)
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
