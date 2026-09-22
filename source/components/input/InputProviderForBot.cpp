#include "components/input/InputProviderForBot.h"
#include "application/GameConfig.h"
#include "components/LineOfSight.h"
#include "entities/BaseObj.h"
#include "entities/obstacles/IFortress.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "enums/Faction.h"
#include "geometry/ObjRectangle.h"
#include "geometry/Point.h"
#include "interfaces/IPickupableBonus.h"
#include "utils/ColliderUtils.h"
#include "utils/DirectionUtils.h"
#include "utils/ObjectUtils.h"
#include "utils/RandUtils.h"
#include <algorithm>
#include <chrono>
#include <optional>
#include <random>

using namespace std::chrono_literals;

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

const Bullet* InputProviderForBot::AsBullet(const std::shared_ptr<BaseObj>& obstacle)
{
	return dynamic_cast<Bullet*>(obstacle.get());
}

//NOTE: the lane a bullet travels, not the distance to it - a shot two rows over is somebody else's
InputProviderForBot::BulletThreat InputProviderForBot::FindBulletThreat(const Tank& self) const
{
	const ObjRectangle selfRect{self.GetRect()};

	BulletThreat nearest{};
	for (const std::shared_ptr<BaseObj>& object: _allObjects)
	{
		if (!ObjectUtils::IsAlive(object) || IsAlly(self, object))
		{
			continue;
		}

		const Bullet* bullet{AsBullet(object)};
		if (bullet == nullptr)
		{
			continue;
		}

		//NOTE: where it is going has to end on us - one flying away or crossing another row is not ours
		//to answer. A negative gap means it is already level with us, and the sweep below would read
		//backwards from there, so it is turned away first
		const Direction flying{bullet->GetDirection()};
		const double gap{DirectionUtils::GapTo(object->GetRect(), selfRect, flying)};
		const double reach{gap + DirectionUtils::SizeAlong(selfRect, flying)};
		if (gap < 0.0
			|| !ColliderUtils::IsCollide(DirectionUtils::Swept(object->GetRect(), reach, flying), selfRect))
		{
			continue;
		}

		const double speed{bullet->GetFlightSpeed()};
		const double timeToImpact{speed > 0.0 ? gap / speed : 0.0};
		if (nearest.bullet && nearest.timeToImpact <= timeToImpact)
		{
			continue;
		}

		nearest = BulletThreat{.bullet = object,
							   .timeToImpact = timeToImpact,
							   .flying = flying,
							   .isHeadOn = flying == DirectionUtils::Opposite(self.GetDirection())};
	}

	return nearest;
}

//NOTE: both ways across, measured to whatever stops us first - the wall of the field counts, a bot
//pressed into it has nowhere to go and should try the other side
std::optional<Direction> InputProviderForBot::SideWithMoreRoom(const Tank& self, const Direction threatDir,
																	   const double deltaTime) const
{
	const std::vector<Direction> freePath{self.GetFreePathSides(deltaTime, std::nullopt)};
	const ObjRectangle selfRect{self.GetRect()};

	std::optional<Direction> best{};
	double bestRoom{};
	for (const Direction side: DirectionUtils::Laterals(threatDir))
	{
		if (std::ranges::find(freePath, side) == freePath.end())
		{
			continue;
		}

		double room{DirectionUtils::GapToEdge(selfRect, _gameConfig.battlefieldSize, side)};
		for (const std::shared_ptr<BaseObj>& object: _allObjects)
		{
			if (!ObjectUtils::IsAlive(object) || object->GetIsPassable() || object.get() == &self)
			{
				continue;
			}

			if (const double gap{DirectionUtils::GapTo(selfRect, object->GetRect(), side)};
				gap >= 0.0 && ColliderUtils::IsCollide(DirectionUtils::Swept(selfRect, gap, side),
													   object->GetRect()))
			{
				room = std::min(room, gap);
			}
		}

		if (!best || room > bestRoom)
		{
			best = side;
			bestRoom = room;
		}
	}

	return best;
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

	//NOTE: a bullet is no longer a reason to turn the hull - it carries its shooter's faction, so it used
	//to read as a tank, and the bot drove at the shot instead of answering it. FindBulletThreat does that
	if (const auto& nearestSeenObstacle{sideObstacle.front()};
		IsOpponent(self, nearestSeenObstacle) && AsBullet(nearestSeenObstacle) == nullptr)
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

	if (nearestSeenObstacle == nullptr)
	{
		nearestSeenObstacle = NearestAhead(lineOfSight, dir);
	}

	return nearestSeenObstacle;
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

bool InputProviderForBot::IsFortress(const std::shared_ptr<BaseObj>& obj)
{
	return dynamic_cast<IFortress*>(obj.get()) != nullptr;
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
	return self.GetFaction() == Faction::EnemyTeam || !IsFortress(obj);
}

//NOTE: asked every frame, so without a cooldown on a refusal any chance fires within a few
//frames. A successful roll needs none - the reload already paces the next shot
bool InputProviderForBot::RollShootObstacle(const std::shared_ptr<BaseObj>& obj)
{
	if (!_obstacleShootCooldown.IsCooldownFinish())
	{
		return false;
	}

	const double chance{IsFortress(obj) ? _gameConfig.botShootFortressChance
										  : _gameConfig.botShootObstacleChance};
	if (RandUtils::GetRandNumber(std::uniform_real_distribution{0.0, 1.0}) < chance)
	{
		return true;
	}

	_obstacleShootCooldown.Reset(_gameConfig.botObstacleShootCooldown);

	return false;
}

//NOTE: the first of the two calls Tank::TickUpdate makes, so the threat found here is the one ShouldShoot
//answers a moment later - intercepting and stepping aside are the two halves of one decision
std::optional<Direction> InputProviderForBot::ChooseDirection(Tank& self, const double deltaTime)
{
	_threat = FindBulletThreat(self);

	//NOTE: the hull is not turned towards the bullet - if it is already head-on there is a shot to take,
	//and if it is not, turning to face it would only drive the bot into it
	if (const bool canIntercept{_threat.isHeadOn && self.CanShoot()
								&& _threat.timeToImpact > kInterceptWindowSeconds};
		_threat.bullet && !canIntercept)
	{
		if (const std::optional<Direction> aside{SideWithMoreRoom(self, _threat.flying, deltaTime)})
		{
			//NOTE: the random turn is pushed back, the way the bonus branch does it - without this the
			//timer fires on the next frame and undoes the dodge
			_randomChangeDirTimer.Reset(RandUtils::GetRandDuration(kMinTurnDelay, kMaxTurnDelay));

			//NOTE: we just turned across the shot, so "head-on" described where we used to look - ShouldShoot
			//would fire the intercept sideways on a reading that is one turn out of date
			_threat.isHeadOn = false;

			return aside;
		}
	}

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
	//NOTE: the answer to a bullet already on our line, decided with the dodge in ChooseDirection - it
	//comes first because nothing else matters while a shot is on its way here
	if (_threat.isHeadOn && _threat.timeToImpact > kInterceptWindowSeconds)
	{
		return true;
	}

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
	return self.CanShoot() && ShouldShootObstacle(self, nearestSeenObstacle) && RollShootObstacle(nearestSeenObstacle);
}
