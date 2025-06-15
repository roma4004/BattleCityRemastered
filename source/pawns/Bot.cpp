#include "../../headers/BonusEffectProperty.h"
#include "../../headers/behavior/MoveLikeTankBeh.h"
#include "../../headers/behavior/ShootingBeh.h"
#include "../../headers/components/LineOfSight.h"
#include "../../headers/enums/Direction.h"
#include "../../headers/interfaces/IPickupableBonus.h"
#include "../../headers/pawns/Enemy.h"
#include "../../headers/pawns/PawnProperty.h"
#include "../../headers/utils/TimeUtils.h"

#include <algorithm>
#include <chrono>

Bot::Bot(PawnProperty pawnProperty, std::shared_ptr<BulletPool> bulletPool, const BonusEffectProperty effects)
	: Tank{pawnProperty,
	       std::make_unique<MoveLikeTankBeh>(this, pawnProperty.allObjects),
	       std::make_shared<ShootingBeh>(this, pawnProperty.allObjects, pawnProperty.events, std::move(bulletPool)),
	       effects
	  },
	  _distDirection(0, 3),
	  _distTurnRate(1000/*ms*/, 5000/*ms*/),
	  _lastTimeTurn{std::chrono::system_clock::now()}
{
	std::random_device rd;
	_gen = std::mt19937(static_cast<unsigned int>(
		std::chrono::high_resolution_clock::now().time_since_epoch().count() + rd()));
}

Bot::~Bot() = default;

bool Bot::IsOpponent(const std::shared_ptr<BaseObj>& obstacle) const
{
	if (const auto tank = dynamic_cast<Tank*>(obstacle.get()))
	{
		return tank->GetFraction() != _fraction;
	}

	return false;
}

bool Bot::IsAlly(const std::shared_ptr<BaseObj>& obstacle) const
{
	if (const auto tank = dynamic_cast<Tank*>(obstacle.get()))
	{
		return tank->GetFraction() == _fraction;
	}

	return false;
}

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
		LineOfSight bonusLOS(_rect, _windowSize, _allObjects, this);
		const std::vector<std::shared_ptr<BaseObj>>& dirSideObstacles =
				[&bonusLOS, dir]() mutable -> std::vector<std::shared_ptr<BaseObj>>&
				{
					if (dir == UP)
					{
						return bonusLOS.GetUpSideObstacles();
					}

					if (dir == LEFT)
					{
						return bonusLOS.GetLeftSideObstacles();
					}

					if (dir == DOWN)
					{
						return bonusLOS.GetDownSideObstacles();
					}

					return bonusLOS.GetRightSideObstacles();
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
	// fire on obstacle if player not found
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
	// change dir when random time span left
	if (TimeUtils::IsCooldownFinish(_lastTimeTurn, _turnDuration))
	{
		_turnDuration = milliseconds(_distTurnRate(_gen));
		const int randDir = _distDirection(_gen);
		SetDirection(static_cast<Direction>(randDir));
		_lastTimeTurn = std::chrono::system_clock::now();
	}

	// move
	const auto pos = GetPos();
	if (_moveBeh->Move(deltaTime))
	{
		++_animationFrameId;
		if (_animationFrameId % 12 && ++_animationId > _animationIdLimit)
		{
			_animationId = 0;
			_animationFrameId = 0;
		}
	}

	// change dir it can't move
	if (pos == GetPos())
	{
		const int randDir = _distDirection(_gen);
		SetDirection(static_cast<Direction>(randDir));
	}
}
