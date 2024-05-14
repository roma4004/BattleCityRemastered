#pragma once

#include "../headers/Pawn.h"

struct FPoint;
struct Environment;

class PlayerOne final : public Pawn
{
public:
	PlayerOne(const FPoint& pos, float width, float height, int color, float speed, int health, Environment* env);
	PlayerOne(const FPoint& pos, int color, Environment* env);

	~PlayerOne() override;

	void KeyboardEvensHandlers(Uint32 eventType, SDL_Keycode key) override;
};