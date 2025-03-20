#include "../../headers/application/Window.h"
#include "../../headers/behavior/MoveLikeAIBeh.h"
#include "../../headers/behavior/ShootingBeh.h"
#include "../../headers/components/LineOfSight.h"
#include "../../headers/enums/Direction.h"
#include "../../headers/interfaces/IPickupableBonus.h"
#include "../../headers/pawns/Enemy.h"
#include "../../headers/pawns/PawnProperty.h"
#include "../../headers/utils/TimeUtils.h"

#include <algorithm>
#include <chrono>

Bot::Bot(PawnProperty pawnProperty, std::shared_ptr<BulletPool> bulletPool)
	: Tank{std::move(pawnProperty),
	       std::make_unique<MoveLikeAIBeh>(this, pawnProperty.allObjects),
	       std::make_shared<ShootingBeh>(this, pawnProperty.allObjects, pawnProperty.events, std::move(bulletPool))
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

bool Bot::IsOpponent(const std::weak_ptr<BaseObj>& obstacle) const
{
	if (const auto obstacleLck = obstacle.lock())
	{
		if (const auto tank = dynamic_cast<Tank*>(obstacleLck.get()))
		{
			return tank->GetFraction() != _fraction;
		}

		return false;
	}

	return false;
}

bool Bot::IsAlly(const std::weak_ptr<BaseObj>& obstacle) const
{
	if (const auto obstacleLck = obstacle.lock())
	{
		if (const auto tank = dynamic_cast<Tank*>(obstacleLck.get()))
		{
			return tank->GetFraction() == _fraction;
		}

		return false;
	}

	return false;
}

bool Bot::IsBonus(const std::weak_ptr<BaseObj>& obstacle)
{
	if (const std::shared_ptr<BaseObj> obstacleLck = obstacle.lock();
		dynamic_cast<IPickupableBonus*>(obstacleLck.get()))
	{
		return true;
	}

	return false;
}

bool Bot::IsFreePathToBonus(const std::vector<std::weak_ptr<BaseObj>>& sideObstacles)
{
	if (const auto nearestObstacleBonus = sideObstacles.front();
		IsBonus(nearestObstacleBonus))
	{
		return true;
	}

	return false;
}

bool Bot::ActIfOpponentSeen(const Direction dir, const std::weak_ptr<BaseObj>& nearestObstacle)
{
	if (IsOpponent(nearestObstacle))
	{
		SetDirection(dir);
		Shot();

		return true;
	}

	return false;
}

bool Bot::ActIfBonusSeen(const Direction dir, const std::weak_ptr<BaseObj>& nearestObstacle)
{
	if (IsBonus(nearestObstacle))
	{
		LineOfSight bonusLOS(_shape, _window->size, _allObjects, this);
		const std::vector<std::weak_ptr<BaseObj>>& dirSideObstacles =
				[&bonusLOS, dir]() mutable -> std::vector<std::weak_ptr<BaseObj>>& {
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

bool Bot::HandleSideObstacles(const Direction dir, const std::vector<std::weak_ptr<BaseObj>>& sideObstacle)
{
	if (!sideObstacle.empty())
	{
		const std::weak_ptr<BaseObj>& nearestObstacle = sideObstacle.front();
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

void Bot::HandleLineOfSight(const Direction dir)
{
	LineOfSight lineOfSight(_shape, _window->size, _bulletSize, _allObjects, this);

	const auto& upSideObstacles = lineOfSight.GetUpSideObstacles();
	if (HandleSideObstacles(UP, upSideObstacles))
	{
		return;
	}

	const auto& leftSideObstacles = lineOfSight.GetLeftSideObstacles();
	if (HandleSideObstacles(LEFT, leftSideObstacles))
	{
		return;
	}

	const auto& downSideObstacles = lineOfSight.GetDownSideObstacles();
	if (HandleSideObstacles(DOWN, downSideObstacles))
	{
		return;
	}

	const auto& rightSideObstacles = lineOfSight.GetRightSideObstacles();
	if (HandleSideObstacles(RIGHT, rightSideObstacles))
	{
		return;
	}

	// TODO: write logic if seen bullet flying toward(head-on) to this tank, need shoot to intercept
	// if (isBullet && isOpposite(bullet->GetDirection))
	// {
	// 	Shot();
	// }

	// fire on obstacle if player not found
	if (dir == UP && !upSideObstacles.empty())
	{
		if (_nearestSeenObstacle = upSideObstacles.front().lock();
			_nearestSeenObstacle)
		{
			_shootDistance = _shape.y - _nearestSeenObstacle->GetY();
			_bulletOffset = _bulletSize.y;
		}
	}

	if (dir == LEFT && !leftSideObstacles.empty())
	{
		if (_nearestSeenObstacle = leftSideObstacles.front().lock();
			_nearestSeenObstacle)
		{
			_shootDistance = _shape.x - _nearestSeenObstacle->GetX();
			_bulletOffset = _bulletSize.x;
		}
	}

	if (dir == DOWN && !downSideObstacles.empty())
	{
		if (_nearestSeenObstacle = downSideObstacles.front().lock();
			_nearestSeenObstacle)
		{
			_shootDistance = _nearestSeenObstacle->GetY() - _shape.y;
			_bulletOffset = _bulletSize.y;
		}
	}

	if (dir == RIGHT && !rightSideObstacles.empty())
	{
		if (_nearestSeenObstacle = rightSideObstacles.front().lock();
			_nearestSeenObstacle)
		{
			_shootDistance = _nearestSeenObstacle->GetX() - _shape.x;
			_bulletOffset = _bulletSize.x;
		}
	}
}

void Bot::TickUpdate(const float deltaTime)
{
	Tank::TickUpdate(deltaTime);

	// change dir when random time span left
	if (TimeUtils::IsCooldownFinish(_lastTimeTurn, _turnDuration))
	{
		_turnDuration = std::chrono::milliseconds(_distTurnRate(_gen));
		const int randDir = _distDirection(_gen);
		SetDirection(static_cast<Direction>(randDir));
		_lastTimeTurn = std::chrono::system_clock::now();
	}

	// move
	const auto pos = GetPos();
	_moveBeh->Move(deltaTime);

	// change dir it cant move
	if (pos == GetPos())
	{
		const int randDir = _distDirection(_gen);
		SetDirection(static_cast<Direction>(randDir));
	}
}
