#pragma once

#include "../headers/Pawn.h"

class PlayerTwo final : public Pawn
{
public:
	PlayerTwo(const Rectangle& rect, int color, float speed, int health, int* windowBuffer, UPoint windowSize,
			  std::vector<std::shared_ptr<BaseObj>>* allPawns, std::shared_ptr<EventSystem> events);

	~PlayerTwo() override;
};
