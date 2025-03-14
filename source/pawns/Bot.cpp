#include "../../headers/EventSystem.h"
#include "../../headers/LineOfSight.h"
#include "../../headers/behavior/MoveLikeAIBeh.h"
#include "../../headers/behavior/ShootingBeh.h"
#include "../../headers/interfaces/IPickupableBonus.h"
#include "../../headers/pawns/Enemy.h"
#include "../../headers/utils/TimeUtils.h"

#include <algorithm>
#include <chrono>

Bot::Bot(const ObjRectangle& rect, const int color, const int health, std::shared_ptr<Window> window,
             const Direction direction, const float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects,
             const std::shared_ptr<EventSystem>& events, std::string name, std::string fraction,
             std::shared_ptr<BulletPool> bulletPool, const GameMode gameMode, const int id)
	: Tank{rect,
	       color,
	       health,
	       std::move(window),
	       direction,
	       speed,
	       allObjects,
	       events,
	       std::make_unique<MoveLikeAIBeh>(this, allObjects),
	       std::make_shared<ShootingBeh>(this, allObjects, events, std::move(bulletPool)),
	       std::move(name),
	       std::move(fraction),
	       gameMode,
	       id},
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
				[&bonusLOS, dir]() mutable
			-> std::vector<std::weak_ptr<BaseObj>>& {
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

void Bot::TickUpdate(const float deltaTime)
{
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
