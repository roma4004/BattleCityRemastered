#include "behavior/MoveLikeTankBeh.h"
#include "components/LineOfSight.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/Direction.h"
#include "interfaces/IPickupableBonus.h"
#include "utils/RandUtils.h"
#include "utils/TimeUtils.h"

Bot::Bot(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, const BonusEffectProperty effects,
         const bool enableByDefault)
	: Tank{std::move(pawnProperty), bulletPool, effects, enableByDefault},
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
	if (const auto& nearestObstacleBonus = sideObstacles.front();
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
					if (dir == Direction::UP)
					{
						return bonusLineOfSight.GetUpSideObstacles();
					}

					if (dir == Direction::LEFT)
					{
						return bonusLineOfSight.GetLeftSideObstacles();
					}

					if (dir == Direction::DOWN)
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
	if (HandleSideObstacles(Direction::UP, upSideObstacles))
	{
		return {};
	}

	const auto& leftSideObstacles = lineOfSight.GetLeftSideObstacles();
	if (HandleSideObstacles(Direction::LEFT, leftSideObstacles))
	{
		return {};
	}

	const auto& downSideObstacles = lineOfSight.GetDownSideObstacles();
	if (HandleSideObstacles(Direction::DOWN, downSideObstacles))
	{
		return {};
	}

	const auto& rightSideObstacles = lineOfSight.GetRightSideObstacles();
	if (HandleSideObstacles(Direction::RIGHT, rightSideObstacles))
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
	if (dir == Direction::UP && !upSideObstacles.empty())
	{
		if (nearestSeenObstacle = upSideObstacles[0];
			nearestSeenObstacle && nearestSeenObstacle != nullptr)
		{
			_shootDistance = _rect.y - (nearestSeenObstacle->GetY() + nearestSeenObstacle->GetHeight());
			_bulletOffset = _bulletSize.y;
		}
	}

	if (dir == Direction::LEFT && !leftSideObstacles.empty())
	{
		if (nearestSeenObstacle = leftSideObstacles[0];
			nearestSeenObstacle && nearestSeenObstacle != nullptr)
		{
			_shootDistance = _rect.x - (nearestSeenObstacle->GetX() + nearestSeenObstacle->GetWidth());
			_bulletOffset = _bulletSize.x;
		}
	}

	if (dir == Direction::DOWN && !downSideObstacles.empty())
	{
		if (nearestSeenObstacle = downSideObstacles[0];
			nearestSeenObstacle && nearestSeenObstacle != nullptr)
		{
			_shootDistance = nearestSeenObstacle->GetY() - (_rect.y + _rect.h);
			_bulletOffset = _bulletSize.y;
		}
	}

	if (dir == Direction::RIGHT && !rightSideObstacles.empty())
	{
		if (nearestSeenObstacle = rightSideObstacles[0];
			nearestSeenObstacle && nearestSeenObstacle != nullptr)
		{
			_shootDistance = nearestSeenObstacle->GetX() - (_rect.x + _rect.w);
			_bulletOffset = _bulletSize.x;
		}
	}

	return nearestSeenObstacle;
}

void Bot::SetRandomDirection(const double deltaTime)
{
	if (const std::vector<Direction> freePath = _moveBeh->GetFreePathSides(deltaTime);
		!freePath.empty())
	{
		const int max = static_cast<int>(freePath.size() - 1);
		const int pathIndex = RandUtils::GetRandNumber(std::uniform_int_distribution{0, max});
		SetDirection(freePath[pathIndex]);
	}
}

void Bot::TickUpdate(const double deltaTime)
{
	if (TimeUtils::IsCooldownFinish(_lastTimeTurn, _turnDuration))// NOTE: bot auto change dir
	{
		SetRandomDirection(deltaTime);

		_turnDuration = milliseconds(RandUtils::GetRandNumber(_distTurnRate));
		_lastTimeTurn = std::chrono::system_clock::now();
	}

	if (const bool isMove = Pawn::Move(deltaTime);
		!isMove)
	{
		SetRandomDirection(deltaTime);// NOTE: change dir it can't move
		_lastTimeTurn = std::chrono::system_clock::now();
	}
}
