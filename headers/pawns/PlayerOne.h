#pragma once

#include "Tank.h"

class IInputProvider;
class BulletPool;

class PlayerOne final : public Tank
{
	void MoveTo(float deltaTime, Direction direction);

	void TickUpdate(float deltaTime) override;

	std::unique_ptr<IInputProvider> _inputProvider;

public:
	PlayerOne(const ObjRectangle& rect, int color, int health, std::shared_ptr<Window> window, Direction direction,
	          float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events,
	          std::string name, std::string fraction, std::unique_ptr<IInputProvider> inputProvider,
	          std::shared_ptr<BulletPool> bulletPool, GameMode gameMode, int id, int tier = 1);

	~PlayerOne() override;
};
