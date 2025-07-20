#include "entities/pawns/CoopBot.h"
#include "entities/obstacles/EagleTile.h"
#include "entities/obstacles/FortressWall.h"
#include "entities/pawns/PawnProperty.h"
#include "utils/TimeUtils.h"

CoopBot::CoopBot(PawnProperty pawnProperty, std::shared_ptr<BulletPool> bulletPool,
                 const BonusEffectProperty effects = {})
	: Bot{std::move(pawnProperty), std::move(bulletPool), effects} {}

CoopBot::~CoopBot() = default;

void CoopBot::TickUpdate(const float deltaTime)
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
			&& !obstacle->GetIsPenetrable()// skip water, ice, bush(Grass) //TODO: rename Grass to BushesTile
			&& !dynamic_cast<FortressWall*>(obstacle)
			&& !dynamic_cast<EagleTile*>(obstacle))
		{
			if (_shootDistance > _bulletDamageRadius + _bulletOffset)
			{
				Shot();
			}
		}
	}
}
