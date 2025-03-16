#pragma once

#include "Tank.h"
#include "../interfaces/IInputProvider.h"

class PlayerTwo final : public Tank
{

	void MoveTo(float deltaTime, Direction direction);

	void TickUpdate(float deltaTime) override;

	std::unique_ptr<IInputProvider> _inputProvider;

public:
	PlayerTwo(const ObjRectangle& rect, int color, int health, std::shared_ptr<Window> window, Direction direction,
	          float speed, std::vector<std::shared_ptr<BaseObj>>* allObjects, std::shared_ptr<EventSystem> events,
	          std::string name, std::string fraction, std::unique_ptr<IInputProvider> inputProvider,
	          std::shared_ptr<BulletPool> bulletPool, GameMode gameMode, int id, int tier = 1);

	~PlayerTwo() override;

	void SendDamageStatistics(const std::string& author, const std::string& fraction) override;
	void TakeDamage(int damage) override;
};
