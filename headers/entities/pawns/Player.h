#pragma once

#include "Tank.h"

class IInputProvider;
class BulletPool;

class Player final : public Tank
{
	using buuid = boost::uuids::uuid;

	std::unique_ptr<IInputProvider> _inputProvider{};

protected:
	void TickUpdate(double deltaTime) override;

	void Enable() override;
	void Disable() const override;
	void Move(Direction direction, double deltaTime, std::vector<std::shared_ptr<BaseObj>>& outCollisions);

public:
	Player(PawnProperty pawnProperty, const std::shared_ptr<BulletPool>& bulletPool,
		   std::unique_ptr<IInputProvider> inputProvider, GameConfig& gameConfig, bool enableByDefault = false);

	~Player() override;
};
