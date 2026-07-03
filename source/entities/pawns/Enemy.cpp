#include "entities/pawns/Enemy.h"
#include "entities/pawns/PawnProperty.h"

//TODO: fix enemy stuck in bricks(local game) looks like 1 pixel issue when finding free path
//TODO: if enemy see bullets they should try or prioritize move aside
Enemy::Enemy(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, const bool enableByDefault)
	: Bot{std::move(pawnProperty), bulletPool, enableByDefault}
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

		//TODO: rename Grass to BushesTile
		if ((obj->GetIsDestructible() || _tier > 2u)
			&& !obj->GetIsPenetrable())// skip water, ice, bush(Grass)
		{
			return true;
		}

		return false;
	};
}

Enemy::~Enemy() = default;
