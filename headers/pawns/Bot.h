#pragma once

#include "Tank.h"

#include <chrono>
#include <random>

class Bot : public Tank
{
protected:
	std::mt19937 _gen;
	std::uniform_int_distribution<> _distDirection;
	std::uniform_int_distribution<> _distTurnRate;
	std::chrono::time_point<std::chrono::system_clock> _lastTimeTurn;
	std::chrono::milliseconds _turnDuration{std::chrono::seconds(2)};

	[[nodiscard]] bool IsOpponent(const std::weak_ptr<BaseObj>& obstacle) const;
	bool IsAlly(const std::weak_ptr<BaseObj>& obstacle) const;
	[[nodiscard]] static bool IsBonus(const std::weak_ptr<BaseObj>& obstacle);
	static bool IsFreePathToBonus(const std::vector<std::weak_ptr<BaseObj>>& sideObstacles);
	bool ActIfOpponentSeen(Direction dir, const std::weak_ptr<BaseObj>& nearestObstacle);
	bool ActIfBonusSeen(Direction dir, const std::weak_ptr<BaseObj>& nearestObstacle);
	bool HandleSideObstacles(Direction dir, const std::vector<std::weak_ptr<BaseObj>>& sideObstacle);
	void TickUpdate(float deltaTime) override;

public:
	Bot(const ObjRectangle& rect, int color, int health, std::shared_ptr<Window> window, Direction direction,
	    float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects, const std::shared_ptr<EventSystem>& events,
	    std::string name, std::string fraction, std::shared_ptr<BulletPool> bulletPool, GameMode gameMode, int id);

	~Bot() override;
};
