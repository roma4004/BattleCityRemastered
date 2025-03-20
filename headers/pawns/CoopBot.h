#pragma once

#include "Bot.h"

#include <random>

class BulletPool;

class CoopBot final : public Bot
{
	void TickUpdate(float deltaTime) override;

public:
	CoopBot(PawnProperty pawnProperty, std::shared_ptr<BulletPool> bulletPool);

	~CoopBot() override;
};
