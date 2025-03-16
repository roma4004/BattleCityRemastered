#pragma once

#include "Bot.h"
#include <random>

class CoopBot final : public Bot
{
	void HandleLineOfSight(Direction dir);

	void TickUpdate(float deltaTime) override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;

public:
	CoopBot(const ObjRectangle& rect, int color, int health, std::shared_ptr<Window> window, Direction direction,
	        float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events,
	        std::string name, std::string fraction, std::shared_ptr<BulletPool> bulletPool, GameMode gameMode, int id,
	        int tier = 1);

	~CoopBot() override;
};
