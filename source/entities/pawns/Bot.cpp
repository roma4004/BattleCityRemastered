#include "application/GameConfig.h"
#include "behavior/MoveLikeTankBeh.h"
#include "components/EventSystem.h"
#include "components/LineOfSight.h"
#include "components/events/AnimationRenderEvents.h"
#include "components/events/ReplicationEvents.h"
#include "entities/pawns/Enemy.h"
#include "entities/pawns/PawnProperty.h"
#include "enums/Direction.h"
#include "enums/GameMode.h"
#include "interfaces/IMoveBeh.h"
#include "interfaces/IPickupableBonus.h"
#include "enums/Faction.h"
#include "utils/RandUtils.h"
#include <optional>

Bot::Bot(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, const GameConfig& gameConfig)
	: Tank{std::move(pawnProperty), bulletPool, gameConfig}
	, _distTurnRate(1000 /*ms*/, 5000 /*ms*/)
{
	_shootTimer.cooldown = std::chrono::seconds{1};
	_randomChangeDirTimer.cooldown = std::chrono::seconds{2};
	_randomChangeDirTimer.Reset();
}

Bot::~Bot() = default;

bool Bot::IsOpponent(const std::shared_ptr<BaseObj>& obstacle) const
{
	return obstacle->GetFaction() != _faction && obstacle->GetFaction() != Faction::Neutral;
}

bool Bot::IsAlly(const std::shared_ptr<BaseObj>& obstacle) const { return obstacle->GetFaction() == _faction; }

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
		LineOfSight bonusLineOfSight(_rect, _allObjects, _gameConfig, false);
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

					// if (dir == Direction::RIGHT)
					// {
					return bonusLineOfSight.GetRightSideObstacles();
					// }
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
	LineOfSight lineOfSight(_rect, _calibre.size, _allObjects, _gameConfig);

	auto dir = GetDirection();
	std::shared_ptr<BaseObj> nearestSeenObstacle{EnemyLookup(lineOfSight, dir)};
	if (nearestSeenObstacle == nullptr)
	{
		nearestSeenObstacle = BonusLookup(lineOfSight, dir);
	}

	// TODO: write logic if seen bullet flying toward(head-on) to this tank, need shoot to intercept
	// if (isBullet(nearestSeenObstacle) && isOpposite(bullet->GetDirection))
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

void Bot::SetRandomDirection(const double deltaTime, const bool excludeCurrentDirection)
{
	const std::optional<Direction> excludeDirection{excludeCurrentDirection ? std::optional{_dir} : std::nullopt};
	const std::vector<Direction> freePath{_moveBeh->GetFreePathSides(deltaTime, excludeDirection)};

	if (!freePath.empty())
	{
		const std::size_t maxIndex{freePath.size() - 1u};
		const auto pathIndex{RandUtils::GetRandNumber(std::uniform_int_distribution<std::size_t>{0u, maxIndex})};
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
		// NOTE: bot got stuck against an obstacle, so pick among the remaining 3 sides, excluding the blocked one
		constexpr bool excludeCurrentDirection{true};
		SetRandomDirection(deltaTime, excludeCurrentDirection);
	}

	if (isMove || oldDir != _dir)
	{
		const FPoint pos = GetPos();
		_events->EmitEvent(AnimationTankUpdateEvent{.name = GetName(), .pos = pos, .dir = _dir});

		if (IsHost(_gameMode))
		{
			_events->EmitEvent(PosChangedEvent{.who = _name, .pos = pos, .dir = _dir, .uuid = _uuid});
		}
	}

	// TODO: cover by unit test isTouchTheIce and ice movement logic
	if (_effects.isTouchTheIce)
	{
		if (auto* moveBeh = dynamic_cast<MoveLikeTankBeh*>(_moveBeh.get());
			moveBeh && moveBeh->ApplyMoveVelocity(deltaTime))
		{
			const FPoint pos = GetPos();
			_events->EmitEvent(AnimationTankUpdateEvent{.name = GetName(), .pos = pos, .dir = _dir});

			if (IsHost(_gameMode))
			{
				_events->EmitEvent(PosChangedEvent{.who = _name, .pos = pos, .dir = _dir, .uuid = _uuid});
			}
		}
	}

	if (!outCollisions.empty())
	{
		HandleBonusPickUp(outCollisions.front());
		outCollisions.clear();
	}

	_effects.isTouchTheBushes = IsTouchBush();
	if (const bool isTouchTheIce = IsTouchIce();
		_effects.isTouchTheIce != isTouchTheIce)
	{
		_effects.isTouchTheIce = isTouchTheIce;
		if (auto* moveBeh = dynamic_cast<MoveLikeTankBeh*>(_moveBeh.get()))
		{
			moveBeh->ResetVelocity();
		}
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
