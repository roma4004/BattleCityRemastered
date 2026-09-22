#pragma once

#include "interfaces/IInputProvider.h"
#include "utils/Timer.h"
#include <chrono>
#include <memory>
#include <optional>
#include <vector>

enum class Direction : char8_t;
class BaseObj;
class GameConfig;
class LineOfSight;
class Bullet;
class Tank;

class InputProviderForBot final : public IInputProvider
{
	//NOTE: an incoming shot, and how long there is to answer it - the whole of the bot's reaction to a
	//bullet rests on these two numbers, so they are found once a frame and read by both halves
	struct BulletThreat final
	{
		std::shared_ptr<BaseObj> bullet{nullptr};
		//NOTE: seconds until it reaches us at its own speed, not a distance - a higher tier flies faster,
		//and the cell size is not a constant of the game either
		double timeToImpact{};
		//NOTE: where it is going, which is the axis a dodge has to leave
		Direction flying{};
		//NOTE: it is coming straight at us, rather than crossing the lane we happen to stand in
		bool isHeadOn{};
	};

	const std::vector<std::shared_ptr<BaseObj>>& _allObjects;
	const GameConfig& _gameConfig;

	//NOTE: milliseconds, not seconds - the roll lands in the unit of its bounds
	static constexpr std::chrono::milliseconds kMinTurnDelay{std::chrono::seconds{1}};
	static constexpr std::chrono::milliseconds kMaxTurnDelay{std::chrono::seconds{5}};
	Timer _randomChangeDirTimer{};

	//NOTE: the drivable pass, built at most once per HandleLineOfSight and shared by all four sides
	std::unique_ptr<LineOfSight> _driveLineOfSight{};

	//NOTE: started by a refusal to fire at a wall, and the refusal stands while it ticks
	Timer _obstacleShootCooldown{};

	//NOTE: found in ChooseDirection and read again in ShouldShoot, which Tank::TickUpdate calls in that
	//order - shooting the bullet down and stepping out of its way are one decision, so they are made once
	BulletThreat _threat{};

	//NOTE: below this there is no time for a bullet of ours to meet one of theirs, so the answer is to
	//move instead. A shot leaves the barrel on the next frame at the earliest, and the two close at the
	//sum of their speeds
	static constexpr double kInterceptWindowSeconds{0.12};

	[[nodiscard]] static bool IsOpponent(const Tank& self, const std::shared_ptr<BaseObj>& obstacle);
	[[nodiscard]] static bool IsAlly(const Tank& self, const std::shared_ptr<BaseObj>& obstacle);
	[[nodiscard]] static bool IsBonus(const std::shared_ptr<BaseObj>& obstacle);
	[[nodiscard]] static const Bullet* AsBullet(const std::shared_ptr<BaseObj>& obstacle);

	[[nodiscard]] BulletThreat FindBulletThreat(const Tank& self) const;

	//NOTE: across the bullet's path, not across our own heading - stepping along the lane it travels is
	//driving into it. Of the two ways out, the one with more room: a dodge into a wall one cell away is
	//standing still with extra steps
	[[nodiscard]] std::optional<Direction> SideWithMoreRoom(const Tank& self, Direction threatDir,
															double deltaTime) const;

	[[nodiscard]] bool ChangeDirIfSeenBonus(Tank& self, Direction dir,
											const std::vector<std::shared_ptr<BaseObj>>& sideObstacle);
	[[nodiscard]] bool ChangeDirIfSeenOpponent(Tank& self, Direction dir,
											   const std::vector<std::shared_ptr<BaseObj>>& sideObstacle);
	//NOTE: what makes a side worth turning to - the two lookups differ by this and nothing else
	using SightTrigger = bool (InputProviderForBot::*)(Tank&, Direction,
													  const std::vector<std::shared_ptr<BaseObj>>&);

	[[nodiscard]] std::shared_ptr<BaseObj> Lookup(Tank& self, LineOfSight& lineOfSight, Direction& dir,
												  SightTrigger trigger);

	//NOTE: a bot must not fire into something closer than its own blast, or the shot takes it too
	[[nodiscard]] static bool IsClearToFire(const Tank& self, Direction dir, const BaseObj& target);

	[[nodiscard]] bool CanDriveToBonus(const Tank& self, Direction dir);

	[[nodiscard]] static std::shared_ptr<BaseObj> NearestAhead(LineOfSight& lineOfSight, Direction dir);

	[[nodiscard]] std::shared_ptr<BaseObj> HandleLineOfSight(Tank& self);

	[[nodiscard]] std::optional<Direction> PickRandomDirection(const Tank& self, double deltaTime,
															   bool excludeCurrentDirection = false);

	[[nodiscard]] static bool ShouldShootOpponent(const Tank& self, const std::shared_ptr<BaseObj>& obj);
	[[nodiscard]] static bool IsFortress(const std::shared_ptr<BaseObj>& obj);
	[[nodiscard]] static bool ShouldShootObstacle(const Tank& self, const std::shared_ptr<BaseObj>& obj);
	[[nodiscard]] bool RollShootObstacle(const std::shared_ptr<BaseObj>& obj);

public:
	InputProviderForBot(const std::vector<std::shared_ptr<BaseObj>>& allObjects, const GameConfig& gameConfig);

	~InputProviderForBot() override;

	[[nodiscard]] std::optional<Direction> ChooseDirection(Tank& self, double deltaTime) override;
	[[nodiscard]] std::optional<Direction> ReviseWhenMoveBlocked(Tank& self, double deltaTime) override;
	[[nodiscard]] bool ShouldShoot(Tank& self) override;
};
