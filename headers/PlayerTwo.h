#pragma once

#include "IInputProvider.h"
#include "Tank.h"

class PlayerTwo final : public Tank
{
public:
	PlayerTwo(const Rectangle& rect, int color, float speed, int health, int* windowBuffer, UPoint windowSize,
	          std::vector<std::shared_ptr<BaseObj>>* allPawns, std::shared_ptr<EventSystem> events, std::string name,
	          std::unique_ptr<IInputProvider>& inputProvider, std::shared_ptr<BulletPool> bulletPool);

	~PlayerTwo() override;

	void Subscribe();
	void Unsubscribe() const;

	void Move(float deltaTime) override;

private:
	std::string _name;
	std::unique_ptr<IInputProvider> _inputProvider;
};
