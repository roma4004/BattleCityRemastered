#pragma once

#include "Tank.h"

class IInputProvider;
class BulletPool;

class Player final : public Tank
{
	std::unique_ptr<IInputProvider> _inputProvider;//TODO: write input provider for bot

	void MoveTo(float deltaTime, Direction dir);

	void TickUpdate(float deltaTime) override;

public:

	Player(PawnProperty pawnProperty, std::shared_ptr<BulletPool> bulletPool, std::unique_ptr<IInputProvider> inputProvider);

	~Player() override;
};
