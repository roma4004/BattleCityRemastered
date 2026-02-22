#pragma once

#include "Bot.h"

class BulletPool;

class CoopBot final : public Bot
{
	void TickUpdate(double deltaTime) override;

public:
	CoopBot(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, BonusEffectProperty effects = {},
			bool enableByDefault = false);

	~CoopBot() override;
};
