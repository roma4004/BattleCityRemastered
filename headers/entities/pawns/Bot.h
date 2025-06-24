#pragma once

#include "Tank.h"
#include <chrono>
#include <random>

struct BonusEffectProperty;
class EventSystem;
class BulletPool;

class Bot : public Tank
{
	using milliseconds = std::chrono::milliseconds;
	using buuid = boost::uuids::uuid;

protected:
	std::uniform_int_distribution<> _distDirection{};
	std::uniform_int_distribution<> _distTurnRate{};
	std::chrono::time_point<std::chrono::system_clock> _lastTimeTurn{};
	milliseconds _turnDuration{std::chrono::seconds(2)};
	std::mt19937 _gen{};//TODO: move random into random manager

	//LOS
	float _shootDistance{0.f};
	float _bulletOffset{0.f};

	[[nodiscard]] bool IsOpponent(const std::shared_ptr<BaseObj>& obstacle) const;
	[[nodiscard]] bool IsAlly(const std::shared_ptr<BaseObj>& obstacle) const;
	[[nodiscard]] static bool IsBonus(const std::shared_ptr<BaseObj>& obstacle);
	[[nodiscard]] static bool IsFreePathToBonus(const std::vector<std::shared_ptr<BaseObj>>& sideObstacles);
	[[nodiscard]] bool ActIfOpponentSeen(Direction dir, const std::shared_ptr<BaseObj>& nearestObstacle);
	[[nodiscard]] bool ActIfBonusSeen(Direction dir, const std::shared_ptr<BaseObj>& nearestObstacle);
	[[nodiscard]] bool HandleSideObstacles(Direction dir, const std::vector<std::shared_ptr<BaseObj>>& sideObstacle);
	[[nodiscard]] std::shared_ptr<BaseObj> HandleLineOfSight(Direction dir);

	void TickUpdate(float deltaTime) override;

public:
	Bot(PawnProperty pawnProperty, std::shared_ptr<BulletPool> bulletPool, BonusEffectProperty effects);

	~Bot() override;
};
