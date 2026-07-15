#pragma once

#include "Bot.h"

class BulletPool;

class CoopBot final : public Bot
{
public:
	CoopBot(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, GameConfig& gameConfig,
			bool enableByDefault = false);

	~CoopBot() override;
};
