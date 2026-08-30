#pragma once

#include "Tank.h"
#include <memory>
#include <vector>

class IInputProvider;
class BulletPool;

class Player final : public Tank
{
	std::unique_ptr<IInputProvider> _inputProvider{};

protected:
	void TickUpdate(double deltaTime) override;

	void Move(Direction direction, double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions);

public:
	Player(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool,
		   std::unique_ptr<IInputProvider> inputProvider, const GameConfig& gameConfig);

	~Player() override;
};
