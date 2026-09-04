#include "components/input/InputProviderForBot.h"
#include "components/LineOfSight.h"
#include "entities/BaseObj.h"
#include "entities/obstacles/IFortress.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "enums/Faction.h"
#include "geometry/ObjRectangle.h"
#include "geometry/Point.h"
#include "interfaces/IPickupableBonus.h"
#include "utils/DirectionUtils.h"
#include "utils/RandUtils.h"
#include <chrono>

//TODO: if enemy see bullets they should try or prioritize move aside
InputProviderForBot::InputProviderForBot(const std::vector<std::shared_ptr<BaseObj>>& allObjects,
										 const GameConfig& gameConfig)
	: _allObjects{allObjects}
	, _gameConfig{gameConfig}
	, _distTurnRate(1000 /*ms*/, 5000 /*ms*/)
{
	_randomChangeDirTimer.cooldown = std::chrono::seconds{2};
	_randomChangeDirTimer.Reset();
}

InputProviderForBot::~InputProviderForBot() = default;

bool InputProviderForBot::IsOpponent(const Tank& self, const std::shared_ptr<BaseObj>& obstacle)
{
	return obstacle->GetFaction() != self.GetFaction() && obstacle->GetFaction() != Faction::Neutral;
}

bool InputProviderForBot::IsAlly(const Tank& self, const std::shared_ptr<BaseObj>& obstacle)
{
	return obstacle->GetFaction() == self.GetFaction();
}

bool InputProviderForBot::IsBonus(const std::shared_ptr<BaseObj>& obstacle)
{
	if (dynamic_cast<IPickupableBonus*>(obstacle.get()))
	{
		return true;
	}

	return false;
}

bool InputProviderForBot::ChangeDirIfSeenBonus(Tank& self, const Direction dir,
											   const std::vector<std::shared_ptr<BaseObj>>& sideObstacle)
{
	if (sideObstacle.empty() || dir == self.GetDirection())
	{
		return false;
	}

	if (IsBonus(sideObstacle.front()))
	{
		if (_driveLineOfSight == nullptr)
		{
			_driveLineOfSight = std::make_unique<LineOfSight>(self.GetRect(), _allObjects, _gameConfig, false);
		}

		//Check free path to bonus
		if (const std::vector<std::shared_ptr<BaseObj>>& directionObstacles = _driveLineOfSight->SideObstacles(dir);
			!directionObstacles.empty() && IsBonus(directionObstacles.front()))
		{
			self.SetDirection(dir);

			_randomChangeDirTimer.Reset(std::chrono::milliseconds(RandUtils::GetRandNumber(_distTurnRate)));

			return true;
		}
	}

	return false;
}

bool InputProviderForBot::ChangeDirIfSeenOpponent(Tank& self, const Direction dir,
												  const std::vector<std::shared_ptr<BaseObj>>& sideObstacle)
{
	if (!self.CanShoot() || sideObstacle.empty())
	{
		return false;
	}

	if (const auto& nearestSeenObstacle = sideObstacle.front();
		IsOpponent(self, nearestSeenObstacle))
	{
		if (dir == self.GetDirection())
		{
			return false;
		}

		UpdateShootDistance(self, dir, nearestSeenObstacle);
		if (_obstacleDistance >= self.GetBulletDamageRadius() + _bulletOffset)
		{
			self.SetDirection(dir);

			return true;
		}
	}

	return false;
}

//NOTE: the sides are tried in this order, and the first one that triggers wins - same as when the
//two lookups were spelled out four blocks each
std::shared_ptr<BaseObj> InputProviderForBot::Lookup(Tank& self, LineOfSight& lineOfSight, Direction& dir,
													  const SightTrigger trigger)
{
	for (const Direction side: {Direction::UP, Direction::LEFT, Direction::DOWN, Direction::RIGHT})
	{
		if (const std::vector<std::shared_ptr<BaseObj>>& sideObstacles = lineOfSight.SideObstacles(side);
			(this->*trigger)(self, side, sideObstacles))
		{
			dir = side;

			return sideObstacles.front();
		}
	}

	return {};
}

void InputProviderForBot::UpdateShootDistance(const Tank& self, const Direction dir,
											  const std::shared_ptr<BaseObj>& nearestSeenObstacle)
{
	const ObjRectangle bullet{.w = self.GetBulletWidth(), .h = self.GetBulletHeight()};

	_obstacleDistance = DirectionUtils::GapTo(self.GetRect(), nearestSeenObstacle->GetRect(), dir);
	_bulletOffset = DirectionUtils::SizeAlong(bullet, dir);
}

std::shared_ptr<BaseObj> InputProviderForBot::HandleLineOfSight(Tank& self)
{
	_driveLineOfSight.reset();

	const FPoint bulletSize{.x = self.GetBulletWidth(), .y = self.GetBulletHeight()};
	LineOfSight lineOfSight(self.GetRect(), bulletSize, _allObjects, _gameConfig);

	auto dir = self.GetDirection();
	std::shared_ptr<BaseObj> nearestSeenObstacle{
			Lookup(self, lineOfSight, dir, &InputProviderForBot::ChangeDirIfSeenOpponent)};
	if (nearestSeenObstacle == nullptr)
	{
		nearestSeenObstacle = Lookup(self, lineOfSight, dir, &InputProviderForBot::ChangeDirIfSeenBonus);
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
	if (const std::vector<std::shared_ptr<BaseObj>>& sideObstacles = lineOfSight.SideObstacles(dir);
		!sideObstacles.empty())
	{
		nearestSeenObstacle = sideObstacles.front();
		UpdateShootDistance(self, dir, nearestSeenObstacle);
	}

	return nearestSeenObstacle;
}

std::optional<Direction> InputProviderForBot::PickRandomDirection(const Tank& self, const double deltaTime,
																  const bool excludeCurrentDirection)
{
	const std::optional<Direction> excludeDirection{
			excludeCurrentDirection ? std::optional{self.GetDirection()} : std::nullopt};
	const std::vector<Direction> freePath{self.GetFreePathSides(deltaTime, excludeDirection)};

	if (freePath.empty())
	{
		return std::nullopt;
	}

	const std::size_t maxIndex{freePath.size() - 1u};
	const auto pathIndex{RandUtils::GetRandNumber(std::uniform_int_distribution<std::size_t>{0u, maxIndex})};

	_randomChangeDirTimer.Reset(std::chrono::milliseconds{RandUtils::GetRandNumber(_distTurnRate)});

	return freePath[pathIndex];
}

bool InputProviderForBot::ShouldShootOpponent(const Tank& self, const std::shared_ptr<BaseObj>& obj)
{
	if (obj == nullptr)
	{
		return false;
	}

	if (IsAlly(self, obj))
	{
		return false;
	}

	if (IsOpponent(self, obj))
	{
		return true;
	}

	return false;
}

//NOTE: the whole of what used to tell an enemy bot from a coop one - a bot on the player team is
//defending the eagle, so it never fires at the fortress
bool InputProviderForBot::ShouldShootObstacle(const Tank& self, const std::shared_ptr<BaseObj>& obj)
{
	if (obj == nullptr || IsAlly(self, obj) || IsBonus(obj))
	{
		return false;
	}

	if ((!obj->GetIsDestructible() && self.GetTier() <= 2u) || obj->GetIsPenetrable())// skip water, ice, bush
	{
		return false;
	}

	//NOTE: the eagle and the walls around it
	return self.GetFaction() == Faction::EnemyTeam || dynamic_cast<IFortress*>(obj.get()) == nullptr;
}

std::optional<Direction> InputProviderForBot::ChooseDirection(Tank& self, const double deltaTime)
{
	if (_randomChangeDirTimer.isActive && _randomChangeDirTimer.IsCooldownFinish())
	{
		_randomChangeDirTimer.isActive = false;
	}

	if (!_randomChangeDirTimer.isActive)// NOTE: bot can change direction by timer
	{
		if (const std::optional<Direction> picked = PickRandomDirection(self, deltaTime))
		{
			return picked;
		}
	}

	//NOTE: a bot is never idle - with no reason to turn it keeps driving the way it was
	return self.GetDirection();
}

std::optional<Direction> InputProviderForBot::ReviseWhenMoveBlocked(Tank& self, const double deltaTime)
{
	// NOTE: bot got stuck against an obstacle, so pick among the remaining 3 sides, excluding the blocked one
	constexpr bool excludeCurrentDirection{true};

	return PickRandomDirection(self, deltaTime, excludeCurrentDirection);
}

bool InputProviderForBot::ShouldShoot(Tank& self)
{
	const std::shared_ptr<BaseObj> nearestSeenObstacle = HandleLineOfSight(self);
	if (_obstacleDistance < self.GetBulletDamageRadius() + _bulletOffset)
	{
		return false;
	}

	//TODO: add feature for bots chance to shoot to obstacle
	//TODO: cover this by test, _shootDistance check
	//TODO: refactor to separated flag isClearToFire mean safe distance
	//TODO: cover this by test, that we can't shoot if on cooldown
	return ShouldShootOpponent(self, nearestSeenObstacle) || ShouldShootObstacle(self, nearestSeenObstacle);
}
