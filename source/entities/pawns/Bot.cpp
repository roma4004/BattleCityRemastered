#include "behavior/MoveLikeTankBeh.h"
#include "components/EventSystem.h"
#include "components/LineOfSight.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "interfaces/IMoveBeh.h"
#include "interfaces/IPickupableBonus.h"
#include "utils/ColliderUtils.h"
#include "utils/RandUtils.h"
#include "utils/TimeUtils.h"

Bot::Bot(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, const bool enableByDefault)
	: Tank{std::move(pawnProperty), bulletPool, enableByDefault}
	, _distTurnRate(1000 /*ms*/, 5000 /*ms*/)
	, _lastTimeTurn{std::chrono::system_clock::now()} {}

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

bool Bot::ChangeDirIfOpponentSeen(const Direction dir, const std::shared_ptr<BaseObj>& nearestObstacle)
{
	if (m_shouldShootStrategy(nearestObstacle))
	{
		if (dir != GetDirection())
		{
			SetDirection(dir);

			_turnDuration = milliseconds(RandUtils::GetRandNumber(_distTurnRate));
			_lastTimeTurn = std::chrono::system_clock::now();
		}

		return true;
	}

	return false;
}

bool Bot::ChangeDirIfBonusSeen(const Direction dir, const std::shared_ptr<BaseObj>& nearestObstacle)
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
			if (dir != GetDirection())
			{
				SetDirection(dir);

				_turnDuration = milliseconds(RandUtils::GetRandNumber(_distTurnRate));
				_lastTimeTurn = std::chrono::system_clock::now();
			}

			return true;
		}
	}

	return false;
}

bool Bot::EnemySideCheck(const Direction dir, const std::vector<std::shared_ptr<BaseObj>>& sideObstacle)
{
	if (sideObstacle.empty() == false
		&& ChangeDirIfOpponentSeen(dir, sideObstacle.front()))
	{
		return true;//TODO: cover this by test, that enemy was seen and change dir to him
	}

	return false;
}

bool Bot::BonusSideCheck(const Direction dir, const std::vector<std::shared_ptr<BaseObj>>& sideObstacle)
{
	if (sideObstacle.empty() == false
		&& ChangeDirIfBonusSeen(dir, sideObstacle.front()))
	{
		return true;//TODO: cover this by test, that bonus was seen and change dir to him
	}

	return false;
}

std::shared_ptr<BaseObj> Bot::HandleLineOfSight(const Direction dir)
{
	LineOfSight lineOfSight(_rect, _windowSize, _calibre.size, _allObjects, this);

	//check for enemies
	const auto& upSideObstacles = lineOfSight.GetUpSideObstacles();
	if (EnemySideCheck(Direction::UP, upSideObstacles))
	{
		return upSideObstacles.front();
	}

	const auto& leftSideObstacles = lineOfSight.GetLeftSideObstacles();
	if (EnemySideCheck(Direction::LEFT, leftSideObstacles))
	{
		return leftSideObstacles.front();
	}

	const auto& downSideObstacles = lineOfSight.GetDownSideObstacles();
	if (EnemySideCheck(Direction::DOWN, downSideObstacles))
	{
		return downSideObstacles.front();
	}

	const auto& rightSideObstacles = lineOfSight.GetRightSideObstacles();
	if (EnemySideCheck(Direction::RIGHT, rightSideObstacles))
	{
		return rightSideObstacles.front();
	}

	//check for bonuses
	if (BonusSideCheck(Direction::UP, upSideObstacles))
	{
		return upSideObstacles.front();
	}

	if (BonusSideCheck(Direction::LEFT, leftSideObstacles))
	{
		return leftSideObstacles.front();
	}

	if (BonusSideCheck(Direction::DOWN, downSideObstacles))
	{
		return downSideObstacles.front();
	}

	if (BonusSideCheck(Direction::RIGHT, rightSideObstacles))
	{
		return rightSideObstacles.front();
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
		if (nearestSeenObstacle = upSideObstacles.front();
			nearestSeenObstacle != nullptr)
		{
			_shootDistance = _rect.y - (nearestSeenObstacle->GetY() + nearestSeenObstacle->GetHeight());
			_bulletOffset = _calibre.size.y;
		}
	}

	if (dir == Direction::LEFT && !leftSideObstacles.empty())
	{
		if (nearestSeenObstacle = leftSideObstacles.front();
			nearestSeenObstacle != nullptr)
		{
			_shootDistance = _rect.x - (nearestSeenObstacle->GetX() + nearestSeenObstacle->GetWidth());
			_bulletOffset = _calibre.size.x;
		}
	}

	if (dir == Direction::DOWN && !downSideObstacles.empty())
	{
		if (nearestSeenObstacle = downSideObstacles.front();
			nearestSeenObstacle != nullptr)
		{
			_shootDistance = nearestSeenObstacle->GetY() - (_rect.y + _rect.h);
			_bulletOffset = _calibre.size.y;
		}
	}

	if (dir == Direction::RIGHT && !rightSideObstacles.empty())
	{
		if (nearestSeenObstacle = rightSideObstacles.front();
			nearestSeenObstacle != nullptr)
		{
			_shootDistance = nearestSeenObstacle->GetX() - (_rect.x + _rect.w);
			_bulletOffset = _calibre.size.x;
		}
	}

	return nearestSeenObstacle;
}

std::vector<Direction> Bot::GetFreePathSides(const double deltaTime) const
{
	std::vector<Direction> freePath;

	constexpr int defaultCollisionReserve{4};
	freePath.reserve(defaultCollisionReserve);

	const float speed = _speed * static_cast<float>(deltaTime);
	const auto [x, y, w, h] = _rect;
	const ObjRectangle tankNextPosRectUp{.x = x, .y = y - speed, .w = w, .h = h + speed};
	const ObjRectangle tankNextPosRectDown{.x = x, .y = y, .w = w, .h = h + speed};
	const ObjRectangle tankNextPosRectLeft{.x = x - speed, .y = y, .w = w + speed, .h = h};
	const ObjRectangle tankNextPosRectRight{.x = x, .y = y, .w = w + speed, .h = h};

	bool isFreeUp{true};
	bool isFreeDown{true};
	bool isFreeLeft{true};
	bool isFreeRight{true};

	for (const std::shared_ptr<BaseObj>& object: *_allObjects)
	{
		if (_uuid == object->GetUuid())
		{
			continue;
		}

		if (isFreeUp && ColliderUtils::IsCollide(tankNextPosRectUp, object->GetRect()))
		{
			if (!object->GetIsPassable()) { isFreeUp = false; }
		}

		if (isFreeDown && ColliderUtils::IsCollide(tankNextPosRectDown, object->GetRect()))
		{
			if (!object->GetIsPassable()) { isFreeDown = false; }
		}

		if (isFreeLeft && ColliderUtils::IsCollide(tankNextPosRectLeft, object->GetRect()))
		{
			if (!object->GetIsPassable()) { isFreeLeft = false; }
		}

		if (isFreeRight && ColliderUtils::IsCollide(tankNextPosRectRight, object->GetRect()))
		{
			if (!object->GetIsPassable()) { isFreeRight = false; }
		}
	}

	if (isFreeUp)
	{
		freePath.emplace_back(Direction::UP);
	}

	if (isFreeDown)
	{
		freePath.emplace_back(Direction::DOWN);
	}

	if (isFreeLeft)
	{
		freePath.emplace_back(Direction::LEFT);
	}

	if (isFreeRight)
	{
		freePath.emplace_back(Direction::RIGHT);
	}

	return freePath;
}

void Bot::SetRandomDirection(const double deltaTime)
{
	if (const std::vector<Direction> freePath = GetFreePathSides(deltaTime);
		!freePath.empty())
	{
		const int max = static_cast<int>(freePath.size() - 1);
		const int pathIndex = RandUtils::GetRandNumber(std::uniform_int_distribution{0, max});
		SetDirection(freePath[pathIndex]);

		_turnDuration = milliseconds(RandUtils::GetRandNumber(_distTurnRate));
		_lastTimeTurn = std::chrono::system_clock::now();
	}
}

void Bot::TickUpdate(const double deltaTime)
{
	const std::shared_ptr<BaseObj> nearestSeenObstacle = HandleLineOfSight(GetDirection());

	std::vector<std::shared_ptr<BaseObj>> outCollisions;
	const Direction oldDir{_dir};

	if (TimeUtils::IsCooldownFinish(_lastTimeTurn, _turnDuration))// NOTE: bot can change direction by timer
	{
		SetRandomDirection(deltaTime);
	}

	const bool isMove = _moveBeh->Move(_dir, deltaTime, outCollisions);
	if (!isMove)
	{
		SetRandomDirection(deltaTime);// NOTE: bot will change their direction if it can't move
	}

	if (isMove || oldDir != _dir)
	{
		//TODO: let the animation manager work with string view
		_events->EmitEvent("AnimationTankUpdate", std::string(GetName()), GetPos(), _dir);

		if (_gameMode == GameMode::PlayAsHost)// NOTE: replication position to the client
		{
			_events->EmitEvent("ServerSend_Pos", _name, GetPos(), _dir, _uuid);
		}
	}

	if (!outCollisions.empty())
	{
		HandleBonusPickUp(outCollisions.front());
		outCollisions.clear();
	}

	if (m_shouldShootStrategy(nearestSeenObstacle))
	{
		Shot();
	}
}
