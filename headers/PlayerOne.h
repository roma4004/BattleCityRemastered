#pragma once

#include "../headers/Pawn.h"

class PlayerOne final : public Pawn
{
public:
	PlayerOne(const Point& pos, int width, int height, int color, float speed, int health, Environment* env);

	~PlayerOne() override;

	void KeyboardEvensHandlers(Environment& env, Uint32 eventType, SDL_Keycode key) override;
};