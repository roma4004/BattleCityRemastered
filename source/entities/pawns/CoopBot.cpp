#include "entities/pawns/CoopBot.h"
#include "entities/obstacles/EagleTile.h"
#include "entities/obstacles/FortressWall.h"
#include "entities/pawns/PawnProperty.h"
#include "utils/TimeUtils.h"

CoopBot::CoopBot(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, const bool enableByDefault)
	: Bot{std::move(pawnProperty), bulletPool, enableByDefault}
{
	m_shouldShootStrategy = [this](const std::shared_ptr<BaseObj>& obj)
	{
		if (obj == nullptr)
		{
			return false;
		}

		if (IsAlly(obj))
		{
			return false;
		}

		if (!TimeUtils::IsCooldownFinish(_lastTimeFire, _fireCooldown)
			|| _shootDistance < _calibre.damageRadius + _bulletOffset/*TODO: + _rect.w*/)
		{
			return false;
		}

		if (IsOpponent(obj))
		{
			return true;
		}

		if ((obj->GetIsDestructible() || _tier > 2u)
			&& !obj->GetIsPenetrable()// skip water, ice, bush(Grass)
			&& !std::dynamic_pointer_cast<FortressWall>(obj)
			&& !std::dynamic_pointer_cast<EagleTile>(obj))
		{
			return true;
		}

		return false;
	};
}

CoopBot::~CoopBot() = default;
