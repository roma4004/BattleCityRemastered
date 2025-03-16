#pragma once

#include "Bot.h"

#include <random>

class Enemy final : public Bot
{
	void HandleLineOfSight(Direction dir);

	void TickUpdate(float deltaTime) override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;
	void TakeDamage(int damage) override;

public:
	Enemy(const ObjRectangle& rect, int color, int health, std::shared_ptr<Window> window, Direction direction,
	      float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects, const std::shared_ptr<EventSystem>& events,
	      std::string name, std::string fraction, std::shared_ptr<BulletPool> bulletPool, GameMode gameMode, int id,
	      int tier = 1);

	~Enemy() override;
};
