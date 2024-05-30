#pragma once

#include "Tank.h"

class PlayerTwo final : public Tank
{
public:
	PlayerTwo(const Rectangle& rect, int color, float speed, int health, int* windowBuffer, UPoint windowSize,
			  std::vector<std::shared_ptr<BaseObj>>* allPawns, std::shared_ptr<EventSystem> events);

	~PlayerTwo() override;
};
