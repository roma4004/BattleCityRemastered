#pragma once

#include "Tank.h"
#include "interfaces/IInputProvider.h"

#include <chrono>
#include <random>

class Enemy : public Tank
{
	std::mt19937 gen;
	std::uniform_int_distribution<> distDirection;
	std::uniform_int_distribution<> distTurnRate;
	std::chrono::time_point<std::chrono::system_clock> lastTimeTurn;
	int turnDuration{2};
	std::unique_ptr<IInputProvider> _inputProvider{nullptr};

public:
	Enemy(const Rectangle& rect, int color, int health, int* windowBuffer, UPoint windowSize, Direction direction,
	      float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events,
	      std::string name, std::string fraction, std::shared_ptr<BulletPool> bulletPool);
	~Enemy() override;

	void Subscribe();
	void Unsubscribe() const;

	[[nodiscard]] static bool IsCollideWith(const Rectangle& r1, const Rectangle& r2);
	[[nodiscard]] static bool IsPlayerVisible(const std::vector<std::weak_ptr<BaseObj>>& obstacles);
	void MayShoot(Direction dir);

	void TickUpdate(float deltaTime) override;
	[[nodiscard]] bool IsTurnCooldownFinish() const;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;;
};
