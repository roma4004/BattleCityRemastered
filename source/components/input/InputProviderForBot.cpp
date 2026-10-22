#include "components/input/InputProviderForBot.h"
#include "application/GameConfig.h"
#include "components/LineOfSight.h"
#include "entities/BaseObj.h"
#include "entities/pawns/Bullet.h"
#include "entities/pawns/Tank.h"
#include "enums/Direction.h"
#include "enums/Faction.h"
#include "geometry/Circle.h"
#include "geometry/ObjRectangle.h"
#include "geometry/Point.h"
#include "utils/ColliderUtils.h"
#include "utils/DirectionUtils.h"
#include "utils/ObjectUtils.h"
#include "utils/RandUtils.h"
#include <algorithm>
#include <chrono>
#include <iterator>
#include <memory>
#include <optional>
#include <random>
#include <ranges>
#include <vector>

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
	return gap - drivenDuringFlight >= self.GetBulletBlastRadius() + self.GetBulletHeight() / 2.0;
}

BaseObj* NearestAhead(LineOfSight& lineOfSight, const Direction dir)
{
	const std::vector<BaseObj*>& obstacles{lineOfSight.SideObstacles(dir)};

	return obstacles.empty() ? nullptr : obstacles.front();
}

//NOTE: what the hull is held still for - an opponent tank, not a wall or a shell (a shell carries a faction too)
bool IsOpponentTankInSights(const Tank& self, const BaseObj& target)
{
	return self.CanShoot() && ObjectUtils::IsOpponent(self, target) && dynamic_cast<const Bullet*>(&target) == nullptr;
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
		return ObjectUtils::IsAlive(object.get()) && object.get() != &bullet && object.get() != &self
			   && dynamic_cast<const Bullet*>(object.get()) == nullptr && !object->GetIsPenetrable()
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
		if (!ObjectUtils::IsAlive(object.get()) || ObjectUtils::IsAlly(self, *object))
		{
			continue;
		}

		const Bullet* bullet{dynamic_cast<const Bullet*>(object.get())};
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
		if (nearest.bulletRect && nearest.timeToImpact <= timeToImpact)
		{
			continue;
		}

		nearest = BulletThreat{.bulletRect = object->GetRect(),
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
	const ObjRectangle bulletRect{*threat.bulletRect};

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
			if (!ObjectUtils::IsAlive(object.get()) || object->GetIsPassable() || object.get() == &self)
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
											   const std::vector<BaseObj*>& sideObstacle)
{
	if (sideObstacle.empty() || dir == self.GetDirection())
	{
		return false;
	}

	if (!ObjectUtils::IsBonus(*sideObstacle.front()) || !CanDriveToBonus(self, dir))
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

	const std::vector<BaseObj*>& obstacles{_driveLineOfSight->SideObstacles(dir)};

	return !obstacles.empty() && ObjectUtils::IsBonus(*obstacles.front());
}

bool InputProviderForBot::ChangeDirIfSeenOpponent(Tank& self, const Direction dir,
												  const std::vector<BaseObj*>& sideObstacle)
{
	if (sideObstacle.empty() || dir == self.GetDirection())
	{
		return false;
	}

	const BaseObj& seen{*sideObstacle.front()};
	if (!IsOpponentTankInSights(self, seen) || !HasNoticed(self, dir, seen) || !IsClearToFire(self, dir, seen))
	{
		return false;
	}

	self.SetDirection(dir);
	PostponeRandomTurn();

	return true;
}

//NOTE: the sides are tried in this order, and the first one that triggers wins
BaseObj* InputProviderForBot::Lookup(Tank& self, LineOfSight& lineOfSight, Direction& dir,
									 const SightTrigger trigger)
{
	for (const Direction side: {Direction::UP, Direction::LEFT, Direction::DOWN, Direction::RIGHT})
	{
		if (const std::vector<BaseObj*>& sideObstacles{lineOfSight.SideObstacles(side)};
			(this->*trigger)(self, side, sideObstacles))
		{
			dir = side;

			return sideObstacles.front();
		}
	}

	return nullptr;
}

//NOTE: the band is read off the heading at the sighting - a shot coming at us moves it one faster
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
	const bool isUnderFire{_threat.bulletRect.has_value()};
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
BaseObj* InputProviderForBot::TurnOntoNearestSeen(Tank& self)
{
	const FPoint bulletSize{.x = self.GetBulletWidth(), .y = self.GetBulletHeight()};
	LineOfSight lineOfSight(self.GetRect(), bulletSize, _allObjects, _gameConfig);

	auto dir{self.GetDirection()};
	BaseObj* seen{Lookup(self, lineOfSight, dir, &InputProviderForBot::ChangeDirIfSeenOpponent)};
	if (seen == nullptr)
	{
		seen = NearestAhead(lineOfSight, dir);
	}

	//NOTE: a clear shot at an opponent comes first - a bonus can wait for the reload, the shot cannot
	const bool hasShot{seen != nullptr && IsOpponentTankInSights(self, *seen) && IsClearToFire(self, dir, *seen)};
	if (!hasShot)
	{
		if (BaseObj* const bonus{Lookup(self, lineOfSight, dir, &InputProviderForBot::ChangeDirIfSeenBonus)})
		{
			seen = bonus;
		}
	}

	//NOTE: kept past the call it would point at objects the sweep may have taken since
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

//NOTE: fire at once only when noticed and centered; an opponent caught by an edge goes to the chance roll like a wall
bool InputProviderForBot::IsOpponentLinedUp(const Tank& self, const BaseObj& target)
{
	const Direction heading{self.GetDirection()};

	return ObjectUtils::IsOpponent(self, target) && HasNoticed(self, heading, target)
		   && IsCenteredOn(self, heading, target);
}

//NOTE: not an ally, breakable, not shot-through, not the eagle a player-side bot defends
bool InputProviderForBot::MayShootObstacle(const Tank& self, const BaseObj& target)
{
	const bool isBreakable{target.GetIsDestructible() || self.GetTier() > 2u};
	const bool isDefended{self.GetFaction() != Faction::EnemyTeam && ObjectUtils::IsFortress(target)};

	return !ObjectUtils::IsAlly(self, target) && isBreakable && !target.GetIsPenetrable() && !isDefended;
}

//NOTE: rolled every frame, so a miss starts a cooldown; a hit needs none, the reload paces it
bool InputProviderForBot::RollShootObstacle(const BaseObj& target)
{
	if (!_obstacleShootCooldown.IsCooldownFinish())
	{
		return false;
	}

	const double chance{ObjectUtils::IsFortress(target)
							? _gameConfig.botShootFortressChance
							: _gameConfig.botShootObstacleChance};
	if (RandUtils::GetRandNumber(std::uniform_real_distribution{0.0, 1.0}) < chance)
	{
		return true;
	}

	_obstacleShootCooldown.Reset(_gameConfig.botObstacleShootCooldown);

	return false;
}

//NOTE: no ally or bonus in the blast at the target, nor at each wall behind it up to the first non-wall
bool InputProviderForBot::SparesAlliesAndBonuses(const Tank& self, const BaseObj& target) const
{
	const std::vector<BaseObj*> spared{
			_allObjects | std::views::transform(ObjectUtils::Raw)
			| std::views::filter([&self](const BaseObj* const object)
			{
				return ObjectUtils::IsAlive(object) && object != &self && dynamic_cast<const Bullet*>(object) == nullptr
					   && (ObjectUtils::IsBonus(*object) || ObjectUtils::IsAlly(self, *object));
			})
			| std::ranges::to<std::vector>()};
	if (spared.empty())
	{
		return true;
	}

	const Direction dir{self.GetDirection()};
	const ObjRectangle selfRect{self.GetRect()};
	//NOTE: the blast centered on the hull's midline, at the face of whatever the shell meets
	const auto isTakenByBlastAt = [&self, &selfRect, dir, &spared](const BaseObj& met)
	{
		const double reach{DirectionUtils::SizeAlong(selfRect, dir) / 2.0
						   + DirectionUtils::GapTo(selfRect, met.GetRect(), dir)};
		const Circle blast{.center = DirectionUtils::Moved(selfRect.Center(), reach, dir),
						   .radius = self.GetBulletBlastRadius()};

		return std::ranges::any_of(spared, [&blast](const BaseObj* const object)
		{
			return ColliderUtils::IsCollide(blast, object->GetRect());
		});
	};

	if (isTakenByBlastAt(target))
	{
		return false;
	}

	if (!ObjectUtils::IsWall(target))
	{
		return true;
	}

	const FPoint bulletSize{.x = self.GetBulletWidth(), .y = self.GetBulletHeight()};
	LineOfSight lineOfSight{selfRect, bulletSize, _allObjects, _gameConfig};
	const std::vector<BaseObj*>& lane{lineOfSight.SideObstacles(dir)};
	const auto at{std::ranges::find(lane, &target)};
	if (at == lane.end())
	{
		return true;
	}

	for (const BaseObj* const met: std::ranges::subrange(std::next(at), lane.end()))
	{
		if (met->GetIsPenetrable())
		{
			continue;
		}

		if (isTakenByBlastAt(*met))
		{
			return false;
		}

		if (!ObjectUtils::IsWall(*met))
		{
			return true;
		}
	}

	return true;
}

bool InputProviderForBot::CanIntercept() const
{
	return _threat.isHeadOn && _threat.timeToImpact > kInterceptWindowSeconds;
}

//NOTE: the first of the two calls Tank::TickUpdate makes - the threat found here is what ShouldShoot answers
std::optional<Direction> InputProviderForBot::ChooseDirection(Tank& self, const double deltaTime)
{
	//NOTE: a reloading bot ignores the opponent it sees - turning onto it only to turn away made it jitter
	if (!self.CanShoot())
	{
		_isLinedUpForShot = false;
	}

	_threat = FindBulletThreat(self);

	//NOTE: the hull is not turned towards the bullet - facing it would only drive the bot into it
	const bool canIntercept{CanIntercept() && self.CanShoot()};

	//NOTE: the shot goes along the hull as it stands, so a random turn now would fire the intercept sideways
	if (_threat.bulletRect && canIntercept)
	{
		PostponeRandomTurn();

		return self.GetDirection();
	}

	//NOTE: only a bot standing in the lane has to leave it - one already driving across is on its way out
	if (_threat.bulletRect && DirectionUtils::IsSameAxis(self.GetDirection(), _threat.flying))
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
	const BaseObj* const target{TurnOntoNearestSeen(self)};
	const bool isInReach{target != nullptr && IsClearToFire(self, self.GetDirection(), *target)};
	//NOTE: asked anew every tick - a target that drove off or a gun on cooldown frees the hull
	_isLinedUpForShot = isInReach && IsOpponentTankInSights(self, *target);

	//NOTE: a reloading bot doesn't hold the line (ChooseDirection frees it); a bonus is driven onto, not shot
	if (!isInReach || !self.CanShoot() || ObjectUtils::IsBonus(*target))
	{
		return false;
	}

	return IsOpponentLinedUp(self, *target)
		   || (MayShootObstacle(self, *target) && SparesAlliesAndBonuses(self, *target) && RollShootObstacle(*target));
}
