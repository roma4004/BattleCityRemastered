#pragma once

#include "Tank.h"
#include <random>
#include <functional>

struct BonusEffectProperty;
class EventSystem;
class BulletPool;

class Bot : public Tank
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

protected:
	std::uniform_int_distribution<> _distTurnRate{};
	//TODO: move to random manager one event on start and then get random by type
	std::chrono::time_point<std::chrono::system_clock> _lastTimeTurn{};
	milliseconds _turnDuration{std::chrono::seconds(2)};

	//LOS
	float _shootDistance{};
	float _bulletOffset{};
	std::function<bool(const std::shared_ptr<BaseObj>&)> m_shouldShootStrategy;

	[[nodiscard]] bool IsOpponent(const std::shared_ptr<BaseObj>& obstacle) const;
	[[nodiscard]] bool IsAlly(const std::shared_ptr<BaseObj>& obstacle) const;
	[[nodiscard]] static bool IsBonus(const std::shared_ptr<BaseObj>& obstacle);
	[[nodiscard]] static bool IsFreePathToBonus(const std::vector<std::shared_ptr<BaseObj>>& sideObstacles);
	[[nodiscard]] bool ChangeDirIfOpponentSeen(Direction dir, const std::shared_ptr<BaseObj>& nearestObstacle);
	[[nodiscard]] bool ChangeDirIfBonusSeen(Direction dir, const std::shared_ptr<BaseObj>& nearestObstacle);
	[[nodiscard]] bool EnemySideCheck(Direction dir, const std::vector<std::shared_ptr<BaseObj>>& sideObstacle);
	[[nodiscard]] bool BonusSideCheck(Direction dir, const std::vector<std::shared_ptr<BaseObj>>& sideObstacle);
	[[nodiscard]] std::shared_ptr<BaseObj> HandleLineOfSight(Direction dir);
	[[nodiscard]] std::vector<Direction> GetFreePathSides(double deltaTime) const;
	void SetRandomDirection(double deltaTime);

	void TickUpdate(double deltaTime) override;

public:
	Bot(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, bool enableByDefault = false);

	~Bot() override;
};
