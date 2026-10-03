#include "components/input/InputProviderForBot.h"
#include "application/GameConfig.h"
#include "components/LineOfSight.h"
#include "entities/BaseObj.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "enums/Faction.h"
#include "geometry/ObjRectangle.h"
#include "geometry/Point.h"
#include "utils/ColliderUtils.h"
#include "utils/DirectionUtils.h"
#include "utils/ObjectUtils.h"
#include "utils/RandUtils.h"
#include <algorithm>
#include <chrono>
#include <optional>
#include <random>

using namespace std::chrono_literals;

namespace
{
//NOTE: a bot must not fire into something closer than its own blast, or the shot takes it too
bool IsClearToFire(const Tank& self, const Direction dir, const BaseObj& target)
{
	const double gap{DirectionUtils::GapTo(self.GetRect(), target.GetRect(), dir)};
	//NOTE: the hull drives on while the shell flies; timed over the whole gap, the spare covers a frame's step
	const double drivenDuringFlight{gap * self.GetSpeed() / self.GetBulletSpeed()};

	//NOTE: the shell blows up mid-length, half its height short of the target; height runs along any flight
	return gap - drivenDuringFlight >= self.GetBulletDamageRadius() + self.GetBulletHeight() / 2.0;
}

std::shared_ptr<BaseObj> NearestAhead(LineOfSight& lineOfSight, const Direction dir)
{
	const std::vector<std::shared_ptr<BaseObj>>& obstacles{lineOfSight.SideObstacles(dir)};

	return obstacles.empty() ? nullptr : obstacles.front();
}

//NOTE: a bot in the player team is defending the eagle, so it never fires at the fortress
bool ShouldShootObstacle(const Tank& self, const std::shared_ptr<BaseObj>& obj)
{
	if (obj == nullptr || ObjectUtils::IsAlly(self, obj))
	{
		return false;
	}

	if ((!obj->GetIsDestructible() && self.GetTier() <= 2u) || obj->GetIsPenetrable())// skip water, ice, bush
	{
		return false;
	}

	//NOTE: the eagle and the walls around it - a player's bot shooting those would lose the match for its own side
	return self.GetFaction() == Faction::EnemyTeam || !ObjectUtils::IsFortress(obj);
}

//NOTE: what the hull is held for - a wall waits, and a shell coming at us is the intercept's business
bool IsOpponentTankInSights(const Tank& self, const std::shared_ptr<BaseObj>& target)
{
	return self.CanShoot() && ObjectUtils::IsOpponent(self, target) && ObjectUtils::AsBullet(target) == nullptr;
}
}//namespace

InputProviderForBot::InputProviderForBot(const std::vector<std::shared_ptr<BaseObj>>& allObjects,
										 const GameConfig& gameConfig)
	: _allObjects{allObjects}
	, _gameConfig{gameConfig}
{
	_randomChangeDirTimer.cooldown = 2s;
	_randomChangeDirTimer.Reset();
}

InputProviderForBot::~InputProviderForBot() = default;

bool InputProviderForBot::IsShotStoppedOnTheWay(const ObjRectangle& corridor, const BaseObj& bullet,
												const Tank& self) const
{
	//NOTE: another bullet in the lane is not a wall - the two may cross before either arrives
	const auto stopsIt = [&corridor, &bullet, &self](const std::shared_ptr<BaseObj>& object)
	{
		return ObjectUtils::IsAlive(object) && object.get() != &bullet && object.get() != &self
			   && ObjectUtils::AsBullet(object) == nullptr && !object->GetIsPenetrable()
			   && ColliderUtils::IsCollide(corridor, object->GetRect());
	};

	return std::ranges::any_of(_allObjects, stopsIt);
}

//NOTE: the lane a bullet travels, not the distance to it - a shot two rows over is somebody else's
InputProviderForBot::BulletThreat InputProviderForBot::FindBulletThreat(const Tank& self) const
{
	const ObjRectangle selfRect{self.GetRect()};

	BulletThreat nearest{};
	for (const std::shared_ptr<BaseObj>& object: _allObjects)
	{
		if (!ObjectUtils::IsAlive(object) || ObjectUtils::IsAlly(self, object))
		{
			continue;
		}

		const Bullet* bullet{ObjectUtils::AsBullet(object)};
		if (bullet == nullptr)
		{
			continue;
		}

		//NOTE: only a bullet flying onto us - at a negative gap it is level already and the sweep reads backwards
		const Direction flying{bullet->GetDirection()};
		const double gap{DirectionUtils::GapTo(object->GetRect(), selfRect, flying)};
		const double reach{gap + DirectionUtils::SizeAlong(selfRect, flying)};
		const ObjRectangle corridor{DirectionUtils::Swept(object->GetRect(), reach, flying)};
		if (gap < 0.0 || !ColliderUtils::IsCollide(corridor, selfRect))
		{
			continue;
		}

		//NOTE: asked after the lane matches - it walks the world, and most bullets are already somebody else's
		if (IsShotStoppedOnTheWay(corridor, *object, self))
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

//NOTE: both ways across, to whatever stops us first - the wall of the field counts too
std::optional<Direction> InputProviderForBot::SideOutOfLane(const Tank& self, const BulletThreat& threat,
															const double deltaTime) const
{
	const std::vector<Direction> freePath{self.GetFreePathSides(deltaTime, std::nullopt)};
	const ObjRectangle selfRect{self.GetRect()};
	const ObjRectangle bulletRect{threat.bullet->GetRect()};

	std::optional<Direction> best{};
	double bestRoom{};
	for (const Direction side: DirectionUtils::Laterals(threat.flying))
	{
		if (std::ranges::find(freePath, side) == freePath.end())
		{
			continue;
		}

		const double needed{DirectionUtils::DistanceOutOfLane(selfRect, bulletRect, side)};
		double room{DirectionUtils::GapToEdge(selfRect, _gameConfig.battlefieldSize, side)};
		for (const std::shared_ptr<BaseObj>& object: _allObjects)
		{
			if (!ObjectUtils::IsAlive(object) || object->GetIsPassable() || object.get() == &self)
			{
				continue;
			}

			//NOTE: not a sweep of the hull up to it - a wall one gap away is touched, never overlapped
			if (const double gap{DirectionUtils::GapTo(selfRect, object->GetRect(), side)};
				gap >= 0.0 && DirectionUtils::OverlapsAcross(selfRect, object->GetRect(), side))
			{
				room = std::min(room, gap);
			}
		}

		//NOTE: only if the whole hull fits out of the lane - a gap too narrow to leave by is not a way out
		if (room < needed)
		{
			continue;
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

	if (!ObjectUtils::IsBonus(sideObstacle.front()) || !CanDriveToBonus(self, dir))
	{
		return false;
	}

	self.SetDirection(dir);
	PostponeRandomTurn();

	return true;
}

//NOTE: the shooting pass sees over water, so the way there is asked again on a drivable pass
bool InputProviderForBot::CanDriveToBonus(const Tank& self, const Direction dir)
{
	if (_driveLineOfSight == nullptr)
	{
		_driveLineOfSight = std::make_unique<LineOfSight>(self.GetRect(), _allObjects, _gameConfig, false);
	}

	const std::vector<std::shared_ptr<BaseObj>>& obstacles{_driveLineOfSight->SideObstacles(dir)};

	return !obstacles.empty() && ObjectUtils::IsBonus(obstacles.front());
}

bool InputProviderForBot::ChangeDirIfSeenOpponent(Tank& self, const Direction dir,
												  const std::vector<std::shared_ptr<BaseObj>>& sideObstacle)
{
	if (!self.CanShoot() || sideObstacle.empty())
	{
		return false;
	}

	//NOTE: a bullet carries its shooter's faction, so it used to read as a tank, and the bot drove at the shot
	if (const auto& nearestSeenObstacle{sideObstacle.front()};
		ObjectUtils::IsOpponent(self, nearestSeenObstacle) && ObjectUtils::AsBullet(nearestSeenObstacle) == nullptr)
	{
		if (dir == self.GetDirection() || !HasNoticed(self, dir, *nearestSeenObstacle))
		{
			return false;
		}

		if (IsClearToFire(self, dir, *nearestSeenObstacle))
		{
			self.SetDirection(dir);
			PostponeRandomTurn();

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

//NOTE: the rung is read off the hull at the moment of the sighting - a shot on its way here moves it one up
InputProviderForBot::NoticeBand InputProviderForBot::NoticeBandFor(const Direction heading, const Direction side,
																   const bool isUnderFire)
{
	std::size_t rung{kNoticeFlankRung};
	if (side == heading)
	{
		rung = kNoticeAheadRung;
	}
	else if (side == DirectionUtils::Opposite(heading))
	{
		rung = kNoticeBehindRung;
	}

	return kNoticeLadder[isUnderFire ? rung - 1u : rung];
}

bool InputProviderForBot::HasNoticed(const Tank& self, const Direction side, const BaseObj& target)
{
	SideNotice& notice{_notices[static_cast<std::size_t>(side)]};
	const bool isUnderFire{_threat.bullet != nullptr};
	const bool isNewTarget{notice.target != target.GetUuid()};
	const bool isEscalation{isUnderFire && !notice.isUnderFire};
	if (isNewTarget || isEscalation)
	{
		notice.target = target.GetUuid();
		notice.isUnderFire = isUnderFire;

		const auto [from, to]{NoticeBandFor(self.GetDirection(), side, isUnderFire)};
		const auto delay{std::chrono::duration_cast<std::chrono::milliseconds>(
				RandUtils::GetRandDuration(from, to) * _gameConfig.botNoticeDelayFactor)};
		notice.delay.Reset(delay);
	}

	return notice.delay.IsCooldownFinish();
}

bool InputProviderForBot::IsCenteredOn(const Tank& self, const Direction dir, const BaseObj& target) const
{
	//NOTE: half a cell either way - a shot from this close to the middle still lands inside the hull
	const double band{_gameConfig.gridOffset / 2.0};
	const FPoint selfCenter{self.GetRect().Center()};
	const FPoint targetCenter{target.GetRect().Center()};
	const bool isDrivingAlongY{DirectionUtils::IsVertical(dir)};
	const double offset{isDrivingAlongY ? targetCenter.x - selfCenter.x : targetCenter.y - selfCenter.y};

	return std::abs(offset) <= band;
}

//NOTE: turns the hull onto an opponent worth a shot or a bonus worth driving to, and returns what is in front
std::shared_ptr<BaseObj> InputProviderForBot::TurnOntoNearestSeen(Tank& self)
{
	const FPoint bulletSize{.x = self.GetBulletWidth(), .y = self.GetBulletHeight()};
	LineOfSight lineOfSight(self.GetRect(), bulletSize, _allObjects, _gameConfig);

	auto dir{self.GetDirection()};
	std::shared_ptr<BaseObj> seen{Lookup(self, lineOfSight, dir, &InputProviderForBot::ChangeDirIfSeenOpponent)};
	if (seen == nullptr)
	{
		seen = NearestAhead(lineOfSight, dir);
	}

	//NOTE: an opponent in the sights holds the hull - the bonus keeps until the reload, the shot does not
	const bool hasShot{seen != nullptr && IsOpponentTankInSights(self, seen) && IsClearToFire(self, dir, *seen)};
	if (!hasShot)
	{
		if (const std::shared_ptr<BaseObj> bonus{
				Lookup(self, lineOfSight, dir, &InputProviderForBot::ChangeDirIfSeenBonus)})
		{
			seen = bonus;
		}
	}

	//NOTE: kept past the call it holds every tank it saw - two bots seeing each other would never be freed
	_driveLineOfSight.reset();

	return seen;
}

void InputProviderForBot::PostponeRandomTurn()
{
	_randomChangeDirTimer.Reset(RandUtils::GetRandDuration(kMinTurnDelay, kMaxTurnDelay));
}

std::optional<Direction> InputProviderForBot::PickRandomDirection(const Tank& self, const double deltaTime,
																  const bool excludeCurrentDirection)
{
	//NOTE: one turn per floor, whoever asks - a bot stuck against a wall used to spin on the spot
	if (!_turnFloor.IsCooldownFinish())
	{
		return std::nullopt;
	}

	const std::optional<Direction> excludeDirection{
			excludeCurrentDirection ? std::optional{self.GetDirection()} : std::nullopt};
	const std::vector<Direction> freePath{self.GetFreePathSides(deltaTime, excludeDirection)};

	if (freePath.empty())
	{
		return std::nullopt;
	}

	const std::size_t maxIndex{freePath.size() - 1u};
	const auto pathIndex{RandUtils::GetRandNumber(std::uniform_int_distribution<std::size_t>{0u, maxIndex})};

	PostponeRandomTurn();
	_turnFloor.Reset(kTurnFloor);

	return freePath[pathIndex];
}

//NOTE: a target only once the two centers line up - caught by a sliver it falls through to the obstacle roll
bool InputProviderForBot::ShouldShootOpponent(const Tank& self, const std::shared_ptr<BaseObj>& obj)
{
	if (obj == nullptr)
	{
		return false;
	}

	if (ObjectUtils::IsAlly(self, obj))
	{
		return false;
	}

	//NOTE: asked only with the gun loaded - reloading, its one answer is to leave the line
	if (ObjectUtils::IsOpponent(self, obj) && self.CanShoot())
	{
		const Direction heading{self.GetDirection()};

		return HasNoticed(self, heading, *obj) && IsCenteredOn(self, heading, *obj);
	}

	return false;
}

//NOTE: asked every frame, so a refusal needs the cooldown - a success does not, the reload paces it
bool InputProviderForBot::RollShootObstacle(const std::shared_ptr<BaseObj>& obj)
{
	if (!_obstacleShootCooldown.IsCooldownFinish())
	{
		return false;
	}

	const double chance{ObjectUtils::IsFortress(obj)
							? _gameConfig.botShootFortressChance
							: _gameConfig.botShootObstacleChance};
	if (RandUtils::GetRandNumber(std::uniform_real_distribution{0.0, 1.0}) < chance)
	{
		return true;
	}

	_obstacleShootCooldown.Reset(_gameConfig.botObstacleShootCooldown);

	return false;
}

bool InputProviderForBot::CanIntercept() const
{
	return _threat.isHeadOn && _threat.timeToImpact > kInterceptWindowSeconds;
}

//NOTE: the first of the two calls Tank::TickUpdate makes - the threat found here is what ShouldShoot answers
std::optional<Direction> InputProviderForBot::ChooseDirection(Tank& self, const double deltaTime)
{
	//NOTE: a reloading bot has no business in the opponent it saw - it used to turn onto it, stop, turn away
	if (!self.CanShoot())
	{
		_isLinedUpForShot = false;
	}

	_threat = FindBulletThreat(self);

	//NOTE: the hull is not turned towards the bullet - facing it would only drive the bot into it
	const bool canIntercept{CanIntercept() && self.CanShoot()};

	//NOTE: the shot goes along the hull as it stands, so a random turn now would fire the intercept sideways
	if (_threat.bullet && canIntercept)
	{
		PostponeRandomTurn();

		return self.GetDirection();
	}

	//NOTE: only a bot standing in the lane has to leave it - one already driving across is on its way out
	if (_threat.bullet && DirectionUtils::IsSameAxis(self.GetDirection(), _threat.flying))
	{
		if (const std::optional<Direction> aside{SideOutOfLane(self, _threat, deltaTime)})
		{
			PostponeRandomTurn();

			//NOTE: the turn made "head-on" describe where we used to look, and ShouldShoot would fire on that
			_threat.isHeadOn = false;

			return aside;
		}
	}

	//NOTE: the hull stands where it was aimed until the shot leaves it - taking one is not driving
	if (_isLinedUpForShot)
	{
		return self.GetDirection();
	}

	if (_randomChangeDirTimer.isActive && _randomChangeDirTimer.IsCooldownFinish())
	{
		_randomChangeDirTimer.isActive = false;
	}

	if (!_randomChangeDirTimer.isActive)
	{
		if (const std::optional<Direction> picked{PickRandomDirection(self, deltaTime)})
		{
			//NOTE: same reading going stale as in the dodge - the hull no longer looks where the threat says
			_threat.isHeadOn = false;

			return picked;
		}
	}

	//NOTE: a bot is never idle - with no reason to turn it keeps driving the way it was
	return self.GetDirection();
}

//NOTE: called when the bot is stuck against an obstacle, so the side it faces is the one side it must not pick
std::optional<Direction> InputProviderForBot::ReviseWhenMoveBlocked(Tank& self, const double deltaTime)
{
	//NOTE: the thing in the way is the thing being aimed at, and a shot needs no path around it
	if (_isLinedUpForShot)
	{
		return std::nullopt;
	}

	constexpr bool excludeCurrentDirection{true};

	const std::optional<Direction> revised{PickRandomDirection(self, deltaTime, excludeCurrentDirection)};
	if (revised)
	{
		//NOTE: the hull turned off the line it was on, and ShouldShoot runs after this in the same tick
		_threat.isHeadOn = false;
	}

	return revised;
}

bool InputProviderForBot::ShouldShoot(Tank& self)
{
	//NOTE: decided with the dodge in ChooseDirection - nothing else matters while a shot is on its way here
	if (CanIntercept())
	{
		return true;
	}

	//NOTE: the hull may have just turned onto it, so the shot goes along the direction it holds now
	const std::shared_ptr<BaseObj> target{TurnOntoNearestSeen(self)};
	if (target == nullptr || !IsClearToFire(self, self.GetDirection(), *target))
	{
		_isLinedUpForShot = false;

		return false;
	}

	//NOTE: asked anew every tick - a target that drove off or a gun on cooldown frees the hull
	_isLinedUpForShot = IsOpponentTankInSights(self, target);

	//NOTE: a bonus is driven onto, not shot - going for it is ChooseDirection's business
	if (ObjectUtils::IsBonus(target))
	{
		return false;
	}

	if (ShouldShootOpponent(self, target))
	{
		return true;
	}

	//NOTE: a refusal keeps the loaded shot for an opponent, so it is asked only with the gun loaded
	return self.CanShoot() && ShouldShootObstacle(self, target) && RollShootObstacle(target);
}
