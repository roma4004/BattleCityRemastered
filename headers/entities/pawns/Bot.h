#pragma once

#include "Tank.h"
#include <random>
#include <functional>

struct BonusEffectProperty;
class EventSystem;
class BulletPool;
class LineOfSight;

class Bot : public Tank
{
	using milliseconds = std::chrono::milliseconds;
	std::uniform_int_distribution<> _distTurnRate{};
	Timer _randomChangeDirTimer{};

	double _obstacleDistance{};
	double _bulletOffset{};

protected:
	std::function<bool(const std::shared_ptr<BaseObj>&)> m_shouldShootToObstacleStrategy;

	[[nodiscard]] bool IsOpponent(const std::shared_ptr<BaseObj>& obstacle) const;
	[[nodiscard]] bool IsAlly(const std::shared_ptr<BaseObj>& obstacle) const;
	[[nodiscard]] static bool IsBonus(const std::shared_ptr<BaseObj>& obstacle);
	[[nodiscard]] bool ChangeDirIfSeenBonus(Direction dir, const std::vector<std::shared_ptr<BaseObj>>& sideObstacle);
	[[nodiscard]] bool ChangeDirIfSeenOpponent(Direction dir,
											   const std::vector<std::shared_ptr<BaseObj>>& sideObstacle);
	[[nodiscard]] std::shared_ptr<BaseObj> EnemyLookup(LineOfSight& lineOfSight, Direction& dir);
	[[nodiscard]] std::shared_ptr<BaseObj> BonusLookup(LineOfSight& lineOfSight, Direction& dir);

	void UpdateShootDistance(Direction dir, const std::shared_ptr<BaseObj>& nearestSeenObstacle);

	[[nodiscard]] std::shared_ptr<BaseObj> HandleLineOfSight();

	void SetRandomDirection(double deltaTime, bool excludeCurrentDirection = false);
	bool ShouldShootOpponent(const std::shared_ptr<BaseObj>& obj) const;

	void TickUpdate(double deltaTime) override;

public:
	Bot(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, const GameConfig& gameConfig);

	~Bot() override;
};
