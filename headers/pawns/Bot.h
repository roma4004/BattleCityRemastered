#pragma once

#include "Tank.h"

#include <chrono>
#include <random>

class EventSystem;
class BulletPool;

class Bot : public Tank
{
protected:
	std::mt19937 _gen;
	std::uniform_int_distribution<> _distDirection;
	std::uniform_int_distribution<> _distTurnRate;
	std::chrono::time_point<std::chrono::system_clock> _lastTimeTurn;
	std::chrono::milliseconds _turnDuration{std::chrono::seconds(2)};

	//LOS
	std::shared_ptr<BaseObj> _nearestSeenObstacle{nullptr};
	float _shootDistance{0.f};
	float _bulletOffset{0.f};

	//TODO: recheck all solution [[nodiscard]]
	[[nodiscard]] bool IsOpponent(const std::weak_ptr<BaseObj>& obstacle) const;
	[[nodiscard]] bool IsAlly(const std::weak_ptr<BaseObj>& obstacle) const;
	[[nodiscard]] static bool IsBonus(const std::weak_ptr<BaseObj>& obstacle);
	[[nodiscard]] static bool IsFreePathToBonus(const std::vector<std::weak_ptr<BaseObj>>& sideObstacles);
	[[nodiscard]] bool ActIfOpponentSeen(Direction dir, const std::weak_ptr<BaseObj>& nearestObstacle);
	[[nodiscard]] bool ActIfBonusSeen(Direction dir, const std::weak_ptr<BaseObj>& nearestObstacle);
	[[nodiscard]] bool HandleSideObstacles(Direction dir, const std::vector<std::weak_ptr<BaseObj>>& sideObstacle);
	void HandleLineOfSight(Direction dir);
	void TickUpdate(float deltaTime) override;

public:
	Bot(const ObjRectangle& rect, int color, int health, std::shared_ptr<Window> window, Direction dir,
	    float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects, const std::shared_ptr<EventSystem>& events,
	    std::string name, std::string fraction, std::shared_ptr<BulletPool> bulletPool, GameMode gameMode, int id,
	    int tier);

	~Bot() override;
};
