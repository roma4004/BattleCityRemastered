#include "behavior/MoveLikeTankBeh.h"
#include "behavior/ShootingBeh.h"
#include "components/EventSystem.h"
#include "components/LineOfSight.h"
#include "entities/BonusEffectProperty.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/Direction.h"
#include "interfaces/IPickupableBonus.h"
#include "utils/RandUtils.h"
#include "utils/TimeUtils.h"
#include <algorithm>
#include <chrono>
#include <iostream>

Bot::Bot(PawnProperty pawnProperty, std::shared_ptr<BulletPool> bulletPool, const BonusEffectProperty effects)
	: Tank{pawnProperty,
	       std::make_unique<MoveLikeTankBeh>(this, pawnProperty.allObjects),
	       std::make_shared<ShootingBeh>(this, pawnProperty.allObjects, pawnProperty.events, std::move(bulletPool)),
	       effects
	  },
	  _distDirection(0, 3),
	  _distTurnRate(1000/*ms*/, 5000/*ms*/),
	  _lastTimeTurn{std::chrono::system_clock::now()} {}

Bot::~Bot() = default;

bool Bot::IsOpponent(const std::shared_ptr<BaseObj>& obstacle) const
{
	return obstacle->GetFraction() != _fraction && obstacle->GetFraction() != "Neutral";
}

bool Bot::IsAlly(const std::shared_ptr<BaseObj>& obstacle) const { return obstacle->GetFraction() == _fraction; }

bool Bot::IsBonus(const std::shared_ptr<BaseObj>& obstacle)
{
	if (dynamic_cast<IPickupableBonus*>(obstacle.get()))
	{
		return true;
	}

	return false;
}

bool Bot::IsFreePathToBonus(const std::vector<std::shared_ptr<BaseObj>>& sideObstacles)
{
	if (const auto nearestObstacleBonus = sideObstacles.front();
		IsBonus(nearestObstacleBonus))
	{
		return true;
	}

	return false;
}

bool Bot::ActIfOpponentSeen(const Direction dir, const std::shared_ptr<BaseObj>& nearestObstacle)
{
	if (IsOpponent(nearestObstacle))
	{
		SetDirection(dir);
		Shot();

		return true;
	}

	return false;
}

bool Bot::ActIfBonusSeen(const Direction dir, const std::shared_ptr<BaseObj>& nearestObstacle)
{
	if (IsBonus(nearestObstacle))
	{
		LineOfSight bonusLineOfSight(_rect, _windowSize, _allObjects, this, false);
		const std::vector<std::shared_ptr<BaseObj>>& dirSideObstacles =
				[&bonusLineOfSight, dir]() mutable -> std::vector<std::shared_ptr<BaseObj>>&
				{
					if (dir == UP)
					{
						return bonusLineOfSight.GetUpSideObstacles();
					}

					if (dir == LEFT)
					{
						return bonusLineOfSight.GetLeftSideObstacles();
					}

					if (dir == DOWN)
					{
						return bonusLineOfSight.GetDownSideObstacles();
					}

					return bonusLineOfSight.GetRightSideObstacles();
				}();

		if (IsFreePathToBonus(dirSideObstacles))
		{
			SetDirection(dir);
		}

		return true;
	}

	return false;
}

bool Bot::HandleSideObstacles(const Direction dir, const std::vector<std::shared_ptr<BaseObj>>& sideObstacle)
{
	if (!sideObstacle.empty())
	{
		const std::shared_ptr<BaseObj>& nearestObstacle = sideObstacle.front();
		if (ActIfOpponentSeen(dir, nearestObstacle))
		{
			return true;
		}

		if (ActIfBonusSeen(dir, nearestObstacle))
		{
			return true;
		}
	}

	return false;
}

std::shared_ptr<BaseObj> Bot::HandleLineOfSight(const Direction dir)
{
	LineOfSight lineOfSight(_rect, _windowSize, _bulletSize, _allObjects, this);

	const auto& upSideObstacles = lineOfSight.GetUpSideObstacles();
	if (HandleSideObstacles(UP, upSideObstacles))
	{
		return {};
	}

	const auto& leftSideObstacles = lineOfSight.GetLeftSideObstacles();
	if (HandleSideObstacles(LEFT, leftSideObstacles))
	{
		return {};
	}

	const auto& downSideObstacles = lineOfSight.GetDownSideObstacles();
	if (HandleSideObstacles(DOWN, downSideObstacles))
	{
		return {};
	}

	const auto& rightSideObstacles = lineOfSight.GetRightSideObstacles();
	if (HandleSideObstacles(RIGHT, rightSideObstacles))
	{
		return {};
	}

	// TODO: write logic if seen bullet flying toward(head-on) to this tank, need shoot to intercept
	// if (isBullet && isOpposite(bullet->GetDirection))
	// {
	// 	Shot();
	// }

	std::shared_ptr<BaseObj> nearestSeenObstacle{nullptr};
	// fire on an obstacle if player not found
	if (dir == UP && !upSideObstacles.empty())
	{
		if (nearestSeenObstacle = upSideObstacles[0];
			nearestSeenObstacle && nearestSeenObstacle.get() != nullptr)
		{
			_shootDistance = _rect.y - (nearestSeenObstacle->GetY() + nearestSeenObstacle->GetHeight());
			_bulletOffset = _bulletSize.y;
		}
	}

	if (dir == LEFT && !leftSideObstacles.empty())
	{
		if (nearestSeenObstacle = leftSideObstacles[0];
			nearestSeenObstacle && nearestSeenObstacle.get() != nullptr)
		{
			_shootDistance = _rect.x - (nearestSeenObstacle->GetX() + nearestSeenObstacle->GetWidth());
			_bulletOffset = _bulletSize.x;
		}
	}

	if (dir == DOWN && !downSideObstacles.empty())
	{
		if (nearestSeenObstacle = downSideObstacles[0];
			nearestSeenObstacle && nearestSeenObstacle.get() != nullptr)
		{
			_shootDistance = nearestSeenObstacle->GetY() - (_rect.y + _rect.h);
			_bulletOffset = _bulletSize.y;
		}
	}

	if (dir == RIGHT && !rightSideObstacles.empty())
	{
		if (nearestSeenObstacle = rightSideObstacles[0];
			nearestSeenObstacle && nearestSeenObstacle.get() != nullptr)
		{
			_shootDistance = nearestSeenObstacle->GetX() - (_rect.x + _rect.w);
			_bulletOffset = _bulletSize.x;
		}
	}

	return nearestSeenObstacle;
}

void Bot::TickUpdate(const float deltaTime)
{
	// NOTE: change dir when random time span left
	if (TimeUtils::IsCooldownFinish(_lastTimeTurn, _turnDuration))
	{
		_turnDuration = milliseconds(RandUtils::GetRandNumber(_distTurnRate));
		SetDirection(static_cast<Direction>(RandUtils::GetRandNumber(_distDirection)));
		_lastTimeTurn = std::chrono::system_clock::now();
	}

	const auto pos = GetPos();

	Pawn::Move(deltaTime);

	if (pos == GetPos())// NOTE: change dir it can't move
	{
		SetDirection(static_cast<Direction>(RandUtils::GetRandNumber(_distDirection)));
	}
}
