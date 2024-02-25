#pragma once

#include "../headers/Pawn.h"

class PlayerOne final : public Pawn
{
public:
  PlayerOne(int x, int y, int width, int height, int color, int speed);

  void KeyboardEvensHandlers(Environment& env, Uint32 eventType, SDL_Keycode key) override;
};
