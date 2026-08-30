#pragma once

#include "Bot.h"
#include <memory>

class BulletPool;

class Enemy final : public Bot
{
public:
	Enemy(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, const GameConfig& gameConfig);

	~Enemy() override;

private:
};
