#pragma once

#include "../headers/Pawn.h"

struct Point;
struct Environment;

class PlayerTwo final : public Pawn
{
public:
	PlayerTwo(const Point& pos, int width, int height, int color, float speed, int health, Environment* env);

	~PlayerTwo() override;

	void KeyboardEvensHandlers(Environment& env, Uint32 eventType, SDL_Keycode key) override;
};