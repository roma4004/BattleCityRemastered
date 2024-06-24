#pragma once

#include "IInputProvider.h"
#include "Tank.h"

class PlayerOne : public Tank
{
public:
	PlayerOne(const Rectangle& rect, int color, float speed, int health, int* windowBuffer, UPoint windowSize,
	          std::vector<std::shared_ptr<BaseObj>>* allPawns, std::shared_ptr<EventSystem> events, std::string name,
	          std::unique_ptr<IInputProvider>& inputProvider);

	~PlayerOne() override;

	void Move(float deltaTime) override;

protected:
	std::string _name;
	std::unique_ptr<IInputProvider> _inputProvider;
};
