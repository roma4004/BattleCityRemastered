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
{
	_shootTimer.cooldown = std::chrono::seconds{1};
	_randomChangeDirTimer.cooldown = std::chrono::seconds{2};
	_randomChangeDirTimer.Reset();
}

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

bool Bot::ChangeDirIfSeenBonus(const Direction dir, const std::vector<std::shared_ptr<BaseObj>>& sideObstacle)
{
	if (sideObstacle.empty())
	{
		return false;
	}

	if (IsBonus(sideObstacle.front()))
	{
		LineOfSight bonusLineOfSight(_rect, _windowSize, _allObjects, this, false);
		const std::vector<std::shared_ptr<BaseObj>>& directionObstacles =
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

		//Check free path to bonus
		if (directionObstacles.empty() == false && IsBonus(directionObstacles.front()))
		{
			if (dir != GetDirection())
			{
				SetDirection(dir);

				_randomChangeDirTimer.Reset(milliseconds(RandUtils::GetRandNumber(_distTurnRate)));

				return true;
			}
		}
	}

	return false;
}

bool Bot::ChangeDirIfSeenOpponent(const Direction dir, const std::vector<std::shared_ptr<BaseObj>>& sideObstacle)
{
	if (_shootTimer.isActive || sideObstacle.empty())
	{
		return false;
	}

	if (const auto& nearestSeenObstacle = sideObstacle.front();
		IsOpponent(nearestSeenObstacle))
	{
		if (dir == GetDirection())
		{
			return false;
		}

		UpdateShootDistance(dir, nearestSeenObstacle);
		if (_obstacleDistance >= _calibre.damageRadius + _bulletOffset)
		{
			SetDirection(dir);

			return true;
		}
	}

	return false;
}

std::shared_ptr<BaseObj> Bot::EnemyLookup(LineOfSight& lineOfSight, Direction& dir)
{
	if (const auto& upSideObstacles = lineOfSight.GetUpSideObstacles();
		ChangeDirIfSeenOpponent(Direction::UP, upSideObstacles))
	{
		dir = Direction::UP;
		return upSideObstacles.front();
	}

	if (const auto& leftSideObstacles = lineOfSight.GetLeftSideObstacles();
		ChangeDirIfSeenOpponent(Direction::LEFT, leftSideObstacles))
	{
		dir = Direction::LEFT;
		return leftSideObstacles.front();
	}

	if (const auto& downSideObstacles = lineOfSight.GetDownSideObstacles();
		ChangeDirIfSeenOpponent(Direction::DOWN, downSideObstacles))
	{
		dir = Direction::DOWN;
		return downSideObstacles.front();
	}

	if (const auto& rightSideObstacles = lineOfSight.GetRightSideObstacles();
		ChangeDirIfSeenOpponent(Direction::RIGHT, rightSideObstacles))
	{
		dir = Direction::RIGHT;
		return rightSideObstacles.front();
	}

	return {};
}

std::shared_ptr<BaseObj> Bot::BonusLookup(LineOfSight& lineOfSight, Direction& dir)
{
	if (const auto& upSideObstacles = lineOfSight.GetUpSideObstacles();
		ChangeDirIfSeenBonus(Direction::UP, upSideObstacles))
	{
		dir = Direction::UP;
		return upSideObstacles.front();
	}

	if (const auto& leftSideObstacles = lineOfSight.GetLeftSideObstacles();
		ChangeDirIfSeenBonus(Direction::LEFT, leftSideObstacles))
	{
		dir = Direction::LEFT;
		return leftSideObstacles.front();
	}

	if (const auto& downSideObstacles = lineOfSight.GetDownSideObstacles();
		ChangeDirIfSeenBonus(Direction::DOWN, downSideObstacles))
	{
		dir = Direction::DOWN;
		return downSideObstacles.front();
	}

	if (const auto& rightSideObstacles = lineOfSight.GetRightSideObstacles();
		ChangeDirIfSeenBonus(Direction::RIGHT, rightSideObstacles))
	{
		dir = Direction::RIGHT;
		return rightSideObstacles.front();
	}

	return {};
}

void Bot::UpdateShootDistance(const Direction dir, const std::shared_ptr<BaseObj>& nearestSeenObstacle)
{
	if (dir == Direction::UP)
	{
		_obstacleDistance = _rect.y - nearestSeenObstacle->GetY() + nearestSeenObstacle->GetHeight();
		_bulletOffset = _calibre.size.y;
	}

	if (dir == Direction::LEFT)
	{
		_obstacleDistance = _rect.x - nearestSeenObstacle->GetX() + nearestSeenObstacle->GetWidth();
		_bulletOffset = _calibre.size.x;
	}

	if (dir == Direction::DOWN)
	{
		_obstacleDistance = nearestSeenObstacle->GetY() - (_rect.y + _rect.h);
		_bulletOffset = _calibre.size.y;
	}

	if (dir == Direction::RIGHT)
	{
		_obstacleDistance = nearestSeenObstacle->GetX() - (_rect.x + _rect.w);
		_bulletOffset = _calibre.size.x;
	}
}

std::shared_ptr<BaseObj> Bot::HandleLineOfSight()
{
	LineOfSight lineOfSight(_rect, _windowSize, _calibre.size, _allObjects, this);

	auto dir = GetDirection();
	std::shared_ptr<BaseObj> nearestSeenObstacle{EnemyLookup(lineOfSight, dir)};
	if (nearestSeenObstacle == nullptr)
	{
		nearestSeenObstacle = BonusLookup(lineOfSight, dir);
	}

	// TODO: write logic if seen bullet flying toward(head-on) to this tank, need shoot to intercept
	// if (isBullet && isOpposite(bullet->GetDirection))
	// {
	// 	Shot();
	// }

	if (nearestSeenObstacle != nullptr)
	{
		return nearestSeenObstacle;
	}

	//finding obstacle to shoot if no priority target
	if (dir == Direction::UP)
	{
		if (const auto& upSideObstacles = lineOfSight.GetUpSideObstacles();
			!upSideObstacles.empty())
		{
			nearestSeenObstacle = upSideObstacles.front();
			UpdateShootDistance(dir, nearestSeenObstacle);
		}
	}
	else if (dir == Direction::LEFT)
	{
		if (const auto& leftSideObstacles = lineOfSight.GetLeftSideObstacles();
			!leftSideObstacles.empty())
		{
			nearestSeenObstacle = leftSideObstacles.front();
			UpdateShootDistance(dir, nearestSeenObstacle);
		}
	}
	else if (dir == Direction::DOWN)
	{
		if (const auto& downSideObstacles = lineOfSight.GetDownSideObstacles();
			!downSideObstacles.empty())
		{
			nearestSeenObstacle = downSideObstacles.front();
			UpdateShootDistance(dir, nearestSeenObstacle);
		}
	}
	else if (dir == Direction::RIGHT)
	{
		if (const auto& rightSideObstacles = lineOfSight.GetRightSideObstacles();
			!rightSideObstacles.empty())
		{
			nearestSeenObstacle = rightSideObstacles.front();
			UpdateShootDistance(dir, nearestSeenObstacle);
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
		const int max{static_cast<int>(freePath.size() - 1)};
		const int pathIndex{RandUtils::GetRandNumber(std::uniform_int_distribution{0, max})};
		SetDirection(freePath[pathIndex]);

		_randomChangeDirTimer.Reset(milliseconds{RandUtils::GetRandNumber(_distTurnRate)});
	}
}

bool Bot::ShouldShootOpponent(const std::shared_ptr<BaseObj>& obj) const
{
	if (obj == nullptr)
	{
		return false;
	}

	if (IsAlly(obj))
	{
		return false;
	}

	if (IsOpponent(obj))
	{
		return true;
	}

	return false;
}

void Bot::TickUpdate(const double deltaTime)
{
	if (_randomChangeDirTimer.isActive && _randomChangeDirTimer.IsCooldownFinish())
	{
		_randomChangeDirTimer.isActive = false;
	}

	if (_shootTimer.isActive && _shootTimer.IsCooldownFinish())
	{
		_shootTimer.isActive = false;
	}

	std::vector<std::shared_ptr<BaseObj>> outCollisions;
	const Direction oldDir{_dir};

	if (!_randomChangeDirTimer.isActive)// NOTE: bot can change direction by timer
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
		FPoint pos = GetPos();
		_events->EmitEvent("AnimationTankUpdate", GetName(), pos, _dir);

		if (_gameMode == GameMode::PlayAsHost)// NOTE: replication position to the client
		{
			_events->EmitEvent("ServerSend_Pos", _name, pos, _dir, _uuid);
		}
	}

	if (!outCollisions.empty())
	{
		HandleBonusPickUp(outCollisions.front());
		outCollisions.clear();
	}

	const std::shared_ptr<BaseObj> nearestSeenObstacle = HandleLineOfSight();
	if (!_shootTimer.isActive && _obstacleDistance >= _calibre.damageRadius + _bulletOffset)
	{
		if (ShouldShootOpponent(nearestSeenObstacle)
			|| m_shouldShootToObstacleStrategy(nearestSeenObstacle))
		{
			//TODO: add feature for bots chance to shoot to obstacle
			//TODO: cover this by test, _shootDistance check
			//TODO: refactor to separated flag isClearToFire mean safe distance
			//TODO: cover this by test, that we can't shoot if on cooldown

			Shot();
		}
	}
}
