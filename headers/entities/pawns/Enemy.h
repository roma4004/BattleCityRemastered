#pragma once

#include "Bot.h"

class BulletPool;

class Enemy final : public Bot
{
	using buuid = boost::uuids::uuid;

	void TickUpdate(double deltaTime) override;

public:
	Enemy(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, BonusEffectProperty effects = {},
	      bool enableByDefault = false);

	~Enemy() override;
};
