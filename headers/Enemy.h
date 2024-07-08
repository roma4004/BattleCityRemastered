#pragma once

#include "Tank.h"

#include <chrono>
#include <random>

class Enemy : public Tank
{
	std::mt19937 _gen;
	std::uniform_int_distribution<> _distDirection;
	std::uniform_int_distribution<> _distTurnRate;
	std::chrono::time_point<std::chrono::system_clock> _lastTimeTurn;
	int _turnDuration{2};

	void Subscribe();
	void Unsubscribe() const;

	[[nodiscard]] static bool IsPlayer(const std::weak_ptr<BaseObj>& obstacle);
	[[nodiscard]] static bool IsBonus(const std::weak_ptr<BaseObj>& obstacle);
	void HandleLineOfSight(Direction dir);

	void TickUpdate(float deltaTime) override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

public:
	Enemy(const Rectangle& rect, int color, int health, int* windowBuffer, UPoint windowSize, Direction direction,
	      float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events,
	      std::string name, std::string fraction, std::shared_ptr<BulletPool> bulletPool);
	~Enemy() override;
};
