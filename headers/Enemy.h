#pragma once

#include "Tank.h"

#include <chrono>
#include <random>

class Enemy : public Tank
{
	std::mt19937 gen;
	std::uniform_int_distribution<> distDirection;
	std::uniform_int_distribution<> distTurnRate;
	std::chrono::time_point<std::chrono::system_clock> lastTimeTurn;
	int turnDuration{2};
	std::string _name;

public:
	Enemy(const Rectangle& rect, int color, float speed, int health, int* windowBuffer, UPoint windowSize,
	      std::vector<std::shared_ptr<BaseObj>>* allPawns, std::shared_ptr<EventSystem> events, std::string name);

	~Enemy() override;
	[[nodiscard]] static bool IsCollideWith(const Rectangle& r1, const Rectangle& r2);
	[[nodiscard]] static bool IsPlayerVisible(const std::vector<std::weak_ptr<BaseObj>>& obstacles, Direction dir);
	void MayShoot(Direction dir);

	void Move(float deltaTime) override;
	[[nodiscard]] bool IsTurnCooldownFinish() const;
};
