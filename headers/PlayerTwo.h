#pragma once

#include "../headers/Pawn.h"

struct Point;
class Environment;

class PlayerTwo final : public Pawn
{
public:
	PlayerTwo(const Point& pos, int width, int height, int color, int speed, int health);

	void KeyboardEvensHandlers(Environment& env, Uint32 eventType, SDL_Keycode key) override;
};
