#pragma once

#include "Bot.h"

class BulletPool;

class Enemy final : public Bot
{
	using buuid = boost::uuids::uuid;

	void TickUpdate(double deltaTime) override;

public:
	Enemy(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool, bool enableByDefault = false);

	~Enemy() override;
};
