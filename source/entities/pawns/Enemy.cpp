#include "entities/pawns/Enemy.h"
#include "entities/pawns/PawnProperty.h"
#include "utils/TimeUtils.h"
#include <algorithm>

//TODO: fix enemy stuck in bricks(local game) looks like 1 pixel issue when finding free path
//TODO: if enemy see bullets they should try or prioritize move aside
Enemy::Enemy(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, const bool enableByDefault)
	: Bot{std::move(pawnProperty), bulletPool, enableByDefault}
{
	m_shouldShootStrategy = [this](const std::shared_ptr<BaseObj>& obj)
	{
		if (obj == nullptr)
		{
			return false;
		}

		//TODO: cover this by test, that ally was seen and not shoot him
		if (IsAlly(obj))
		{
			return false;
		}

		//TODO: move timer check to timerManager and onEvent change the class field bool isOnCooldown{false};
		//TODO: cover this by test, _shootDistance check
		//TODO: refactor to separated flag isClearToFire mean safe distance
		//TODO: cover this by test, that we can't shoot if on cooldown
		//TODO: cover pickup bonusTank after lose fortress
		if (!TimeUtils::IsCooldownFinish(_lastTimeFire, _fireCooldown)
			|| _shootDistance < _calibre.damageRadius + _bulletOffset)
		{
			return false;
		}

		//TODO: cover this by test, that enemy was seen and shoot him
		if (IsOpponent(obj))
		{
			return true;
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
