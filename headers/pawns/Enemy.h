#pragma once

#include "Bot.h"

#include <random>

class BulletPool;

class Enemy final : public Bot
{
	using buuid = boost::uuids::uuid;

	void TickUpdate(float deltaTime) override;

public:
	Enemy(PawnProperty pawnProperty, std::shared_ptr<BulletPool> bulletPool);

	~Enemy() override;
};
