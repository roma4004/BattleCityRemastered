#include "entities/pawns/Enemy.h"
#include "entities/pawns/PawnProperty.h"

//TODO: if enemy see bullets they should try or prioritize move aside
Enemy::Enemy(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, const GameConfig& gameConfig)
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
			&& !obj->GetIsPenetrable())// skip water, ice, bush
		{
			return true;
		}

		return false;
	};
}

Enemy::~Enemy() = default;
