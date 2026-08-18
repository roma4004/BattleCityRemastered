#pragma once

#include "Bot.h"

class BulletPool;

class Enemy final : public Bot
{
public:
	Enemy(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, GameConfig& gameConfig);

	~Enemy() override;

private:
};
