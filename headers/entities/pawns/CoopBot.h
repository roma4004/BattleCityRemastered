#pragma once

#include "Bot.h"
#include <memory>

class BulletPool;

class CoopBot final : public Bot
{
public:
	CoopBot(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, const GameConfig& gameConfig);

	~CoopBot() override;
};
