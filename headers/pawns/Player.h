#pragma once

#include "Tank.h"

class IInputProvider;
class BulletPool;

class Player final : public Tank
{
	using buuid = boost::uuids::uuid;

	std::unique_ptr<IInputProvider> _inputProvider;

	void Move(Direction dir, float deltaTime);

	void TickUpdate(float deltaTime) override;

public:
	Player(PawnProperty pawnProperty, std::shared_ptr<BulletPool> bulletPool,
	       std::unique_ptr<IInputProvider> inputProvider, BonusEffectProperty effects);

	~Player() override;
};
