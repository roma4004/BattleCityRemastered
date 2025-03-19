#pragma once

#include "Tank.h"

class IInputProvider;
class BulletPool;

class PlayerTwo final : public Tank
{
	std::unique_ptr<IInputProvider> _inputProvider;

	void MoveTo(float deltaTime, Direction dir);

	void TickUpdate(float deltaTime) override;

public:
	PlayerTwo(const ObjRectangle& rect, int color, int health, std::shared_ptr<Window> window, Direction dir,
	          float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events,
	          std::string name, std::string fraction, std::unique_ptr<IInputProvider> inputProvider,
	          std::shared_ptr<BulletPool> bulletPool, GameMode gameMode, int id, int tier = 1);

	~PlayerTwo() override;
};
