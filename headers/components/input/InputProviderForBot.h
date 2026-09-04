#pragma once

#include "interfaces/IInputProvider.h"
#include "utils/Timer.h"
#include <memory>
#include <optional>
#include <random>
#include <vector>

enum class Direction : char8_t;
class BaseObj;
class GameConfig;
class LineOfSight;
class Tank;

//NOTE: the turn timer, the sighting distance and the line-of-sight pass - state of a decision, not of a tank
class InputProviderForBot final : public IInputProvider
{
	const std::vector<std::shared_ptr<BaseObj>>& _allObjects;
	const GameConfig& _gameConfig;

	std::uniform_int_distribution<> _distTurnRate;
	Timer _randomChangeDirTimer{};

	double _obstacleDistance{};
	double _bulletOffset{};

	//NOTE: the drivable pass, built at most once per HandleLineOfSight and shared by all four sides
	std::unique_ptr<LineOfSight> _driveLineOfSight{};

	[[nodiscard]] static bool IsOpponent(const Tank& self, const std::shared_ptr<BaseObj>& obstacle);
	[[nodiscard]] static bool IsAlly(const Tank& self, const std::shared_ptr<BaseObj>& obstacle);
	[[nodiscard]] static bool IsBonus(const std::shared_ptr<BaseObj>& obstacle);

	[[nodiscard]] bool ChangeDirIfSeenBonus(Tank& self, Direction dir,
											const std::vector<std::shared_ptr<BaseObj>>& sideObstacle);
	[[nodiscard]] bool ChangeDirIfSeenOpponent(Tank& self, Direction dir,
											   const std::vector<std::shared_ptr<BaseObj>>& sideObstacle);
	//NOTE: what makes a side worth turning to - the two lookups differ by this and nothing else
	using SightTrigger = bool (InputProviderForBot::*)(Tank&, Direction,
													  const std::vector<std::shared_ptr<BaseObj>>&);

	[[nodiscard]] std::shared_ptr<BaseObj> Lookup(Tank& self, LineOfSight& lineOfSight, Direction& dir,
												  SightTrigger trigger);

	void UpdateShootDistance(const Tank& self, Direction dir, const std::shared_ptr<BaseObj>& nearestSeenObstacle);

	[[nodiscard]] std::shared_ptr<BaseObj> HandleLineOfSight(Tank& self);

	[[nodiscard]] std::optional<Direction> PickRandomDirection(const Tank& self, double deltaTime,
															   bool excludeCurrentDirection = false);

	[[nodiscard]] static bool ShouldShootOpponent(const Tank& self, const std::shared_ptr<BaseObj>& obj);
	[[nodiscard]] static bool ShouldShootObstacle(const Tank& self, const std::shared_ptr<BaseObj>& obj);

public:
	InputProviderForBot(const std::vector<std::shared_ptr<BaseObj>>& allObjects, const GameConfig& gameConfig);

	~InputProviderForBot() override;

	[[nodiscard]] std::optional<Direction> ChooseDirection(Tank& self, double deltaTime) override;
	[[nodiscard]] std::optional<Direction> ReviseWhenMoveBlocked(Tank& self, double deltaTime) override;
	[[nodiscard]] bool ShouldShoot(Tank& self) override;
};
