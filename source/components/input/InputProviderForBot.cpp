#include "components/input/InputProviderForBot.h"
#include "application/GameConfig.h"
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
#include <random>

using namespace std::chrono_literals;

//TODO: if enemy see bullets they should try or prioritize move aside
InputProviderForBot::InputProviderForBot(const std::vector<std::shared_ptr<BaseObj>>& allObjects,
										 const GameConfig& gameConfig)
	: _allObjects{allObjects}
	, _gameConfig{gameConfig}
{
	_randomChangeDirTimer.cooldown = 2s;
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

	if (!IsBonus(sideObstacle.front()) || !CanDriveToBonus(self, dir))
	{
		return false;
	}

	self.SetDirection(dir);

	_randomChangeDirTimer.Reset(RandUtils::GetRandDuration(kMinTurnDelay, kMaxTurnDelay));

	return true;
}

//NOTE: the shooting pass sees the bonus through what a bullet flies over, so the way there is asked again
//on a drivable pass - a bonus across water is seen and not reachable
bool InputProviderForBot::CanDriveToBonus(const Tank& self, const Direction dir)
{
	if (_driveLineOfSight == nullptr)
	{
		_driveLineOfSight = std::make_unique<LineOfSight>(self.GetRect(), _allObjects, _gameConfig, false);
	}

	const std::vector<std::shared_ptr<BaseObj>>& obstacles{_driveLineOfSight->SideObstacles(dir)};

	return !obstacles.empty() && IsBonus(obstacles.front());
}

bool InputProviderForBot::ChangeDirIfSeenOpponent(Tank& self, const Direction dir,
												  const std::vector<std::shared_ptr<BaseObj>>& sideObstacle)
{
	if (!self.CanShoot() || sideObstacle.empty())
	{
		return false;
	}

	if (const auto& nearestSeenObstacle{sideObstacle.front()};
		IsOpponent(self, nearestSeenObstacle))
	{
		if (dir == self.GetDirection())
		{
			return false;
		}

		if (IsClearToFire(self, dir, *nearestSeenObstacle))
		{
			self.SetDirection(dir);

			return true;
		}
	}

	return false;
}

//NOTE: the sides are tried in this order, and the first one that triggers wins
std::shared_ptr<BaseObj> InputProviderForBot::Lookup(Tank& self, LineOfSight& lineOfSight, Direction& dir,
													  const SightTrigger trigger)
{
	for (const Direction side: {Direction::UP, Direction::LEFT, Direction::DOWN, Direction::RIGHT})
	{
		if (const std::vector<std::shared_ptr<BaseObj>>& sideObstacles{lineOfSight.SideObstacles(side)};
			(this->*trigger)(self, side, sideObstacles))
		{
			dir = side;

			return sideObstacles.front();
		}
	}

	return {};
}

bool InputProviderForBot::IsClearToFire(const Tank& self, const Direction dir, const BaseObj& target)
{
	const ObjRectangle bullet{.w = self.GetBulletWidth(), .h = self.GetBulletHeight()};
	const double gap{DirectionUtils::GapTo(self.GetRect(), target.GetRect(), dir)};

	return gap >= self.GetBulletDamageRadius() + DirectionUtils::SizeAlong(bullet, dir);
}

std::shared_ptr<BaseObj> InputProviderForBot::HandleLineOfSight(Tank& self)
{
	_driveLineOfSight.reset();

	const FPoint bulletSize{.x = self.GetBulletWidth(), .y = self.GetBulletHeight()};
	LineOfSight lineOfSight(self.GetRect(), bulletSize, _allObjects, _gameConfig);

	auto dir{self.GetDirection()};
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

	return NearestAhead(lineOfSight, dir);
}

//NOTE: nothing worth turning for, so the bot keeps its heading and takes whatever stands in it - that is
//what it ends up shooting at
std::shared_ptr<BaseObj> InputProviderForBot::NearestAhead(LineOfSight& lineOfSight, const Direction dir)
{
	const std::vector<std::shared_ptr<BaseObj>>& obstacles{lineOfSight.SideObstacles(dir)};

	return obstacles.empty() ? nullptr : obstacles.front();
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

	_randomChangeDirTimer.Reset(RandUtils::GetRandDuration(kMinTurnDelay, kMaxTurnDelay));

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

//NOTE: a bot in the player team is defending the eagle, so it never fires at the fortress
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

	//NOTE: the eagle and the walls around it - a player's bot shooting those would lose the match for its own side
	return self.GetFaction() == Faction::EnemyTeam || dynamic_cast<IFortress*>(obj.get()) == nullptr;
}

//NOTE: asked every frame, so without a cooldown on a refusal any chance fires within a few
//frames. A successful roll needs none - the reload already paces the next shot
bool InputProviderForBot::RollShootObstacle()
{
	if (!_obstacleShootCooldown.IsCooldownFinish())
	{
		return false;
	}

	if (RandUtils::GetRandNumber(std::uniform_real_distribution{0.0, 1.0}) < _gameConfig.botShootObstacleChance)
	{
		return true;
	}

	_obstacleShootCooldown.Reset(_gameConfig.botObstacleShootCooldown);

	return false;
}

std::optional<Direction> InputProviderForBot::ChooseDirection(Tank& self, const double deltaTime)
{
	if (_randomChangeDirTimer.isActive && _randomChangeDirTimer.IsCooldownFinish())
	{
		_randomChangeDirTimer.isActive = false;
	}

	if (!_randomChangeDirTimer.isActive)
	{
		if (const std::optional<Direction> picked{PickRandomDirection(self, deltaTime)})
		{
			return picked;
		}
	}

	//NOTE: a bot is never idle - with no reason to turn it keeps driving the way it was
	return self.GetDirection();
}

//NOTE: called when the bot is stuck against an obstacle, so the side it faces is the one side it must not pick
std::optional<Direction> InputProviderForBot::ReviseWhenMoveBlocked(Tank& self, const double deltaTime)
{
	constexpr bool excludeCurrentDirection{true};

	return PickRandomDirection(self, deltaTime, excludeCurrentDirection);
}

bool InputProviderForBot::ShouldShoot(Tank& self)
{
	const std::shared_ptr<BaseObj> nearestSeenObstacle{HandleLineOfSight(self)};
	if (nearestSeenObstacle == nullptr)
	{
		return false;
	}

	//NOTE: HandleLineOfSight leaves the tank facing what it found, so its direction is the shot's
	if (!IsClearToFire(self, self.GetDirection(), *nearestSeenObstacle))
	{
		return false;
	}

	if (ShouldShootOpponent(self, nearestSeenObstacle))
	{
		return true;
	}

	//NOTE: a refusal keeps the loaded shot for an opponent, so it is asked only with the gun loaded
	return self.CanShoot() && ShouldShootObstacle(self, nearestSeenObstacle) && RollShootObstacle();
}
