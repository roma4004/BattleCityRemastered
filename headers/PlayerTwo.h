#pragma once

#include "../headers/Pawn.h"

struct FPoint;
struct Environment;

class PlayerTwo final : public Pawn
{
public:
	PlayerTwo(const FPoint& pos, float width, float height, int color, float speed, int health, Environment* env);
	PlayerTwo(const FPoint& pos, int color, Environment* env);

	~PlayerTwo() override;

	void KeyboardEvensHandlers(Uint32 eventType, SDL_Keycode key) override;
};