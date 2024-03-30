#pragma once

#include "../headers/Pawn.h"

class PlayerOne final : public Pawn
{
public:
	PlayerOne(const Point& pos, int width, int height, int color, float speed, int health, Environment* env);
	PlayerOne(const Point& pos, int color, Environment* env);

	~PlayerOne() override;

	void KeyboardEvensHandlers(Uint32 eventType, SDL_Keycode key) override;
};