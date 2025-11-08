#pragma once

#include "Tank.h"

class IInputProvider;
class BulletPool;

class Player final : public Tank
{
	using buuid = boost::uuids::uuid;

	std::unique_ptr<IInputProvider> _inputProvider{};

	void TickUpdate(float deltaTime) override;

public:
	Player(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool,
	       std::unique_ptr<IInputProvider> inputProvider, BonusEffectProperty effects = {},
	       bool enableByDefault = false);

	~Player() override;
};
