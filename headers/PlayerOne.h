#pragma once

#include "../headers/Pawn.h"

class PlayerOne : public Pawn
{
public:
	PlayerOne(const Rectangle& rect, int color, float speed, int health, int* windowBuffer, UPoint windowSize,
			  std::vector<std::shared_ptr<BaseObj>>* allPawns, std::shared_ptr<EventSystem> events);

	~PlayerOne() override;
};
