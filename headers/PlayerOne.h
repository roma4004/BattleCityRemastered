#pragma once

#include "../headers/Pawn.h"

class PlayerOne final : public Pawn {
public:
	PlayerOne(const Point& pos, int width, int height, int color, int speed,
			  int health);

	void KeyboardEvensHandlers(Environment &env, Uint32 eventType,
							   SDL_Keycode key) override;
};
