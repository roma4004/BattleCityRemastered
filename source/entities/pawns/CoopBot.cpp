#include "entities/pawns/CoopBot.h"
#include "entities/obstacles/EagleTile.h"
#include "entities/obstacles/FortressWall.h"
#include "entities/pawns/PawnProperty.h"

CoopBot::CoopBot(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, GameConfig& gameConfig)
	: Bot{std::move(pawnProperty), bulletPool, gameConfig}
{
	m_shouldShootToObstacleStrategy = [this](const std::shared_ptr<BaseObj>& obj)
	{
		if (obj == nullptr)
		{
			return false;
		}

		if (IsAlly(obj) || IsBonus(obj))
		{
			return false;
		}

		if ((obj->GetIsDestructible() || _tier > 2u)
			&& !obj->GetIsPenetrable()// skip water, ice, bush
			&& !std::dynamic_pointer_cast<FortressWall>(obj)
			&& !std::dynamic_pointer_cast<EagleTile>(obj))
		{
			return true;
		}

		return false;
	};

	_shootTimer.cooldown = std::chrono::milliseconds{500};
}

CoopBot::~CoopBot() = default;
