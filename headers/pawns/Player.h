#pragma once

#include "Tank.h"

class IInputProvider;
class BulletPool;

class Player final : public Tank
{
	std::unique_ptr<IInputProvider> _inputProvider;//TODO: write input provider for bot

	void Move(Direction dir, float deltaTime);

	void TickUpdate(float deltaTime) override;

public:
	Player(PawnProperty pawnProperty, std::shared_ptr<BulletPool> bulletPool,
	       std::unique_ptr<IInputProvider> inputProvider);

	~Player() override;
};
